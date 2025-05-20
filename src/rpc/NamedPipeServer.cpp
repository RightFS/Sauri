#include "NamedPipeServer.h"

NamedPipeServer::NamedPipeServer(boost::asio::io_context &io_context, const std::string &pipe_name)
        : io_context_(io_context),
          pipe_name_("\\\\.\\pipe\\" + pipe_name),
          strand_(io_context.get_executor()),
          pipe_(io_context),
          timer_(io_context),
          is_connected_(false),
          is_stopped_(false) {

    // Default handlers
    on_message_ = [](const std::string &message) {
        std::cout << "Received: " << message << std::endl;
    };

    on_error_ = [](const error_code &ec) {
        std::cerr << "Error: " << ec.message() << std::endl;
    };

    on_connect_ = []() {
        std::cout << "Client connected" << std::endl;
    };

    on_disconnect_ = []() {
        std::cout << "Client disconnected" << std::endl;
    };
}

NamedPipeServer::~NamedPipeServer() {
    stop();
}

void NamedPipeServer::set_message_handler(message_handler handler) {
    on_message_ = handler;
}

void NamedPipeServer::set_error_handler(error_handler handler) {
    on_error_ = handler;
}

void NamedPipeServer::set_connect_handler(connect_handler handler) {
    on_connect_ = handler;
}

void NamedPipeServer::set_disconnect_handler(disconnect_handler handler) {
    on_disconnect_ = handler;
}

void NamedPipeServer::start() {
    if (is_stopped_) {
        return;
    }

    try {
        // Create a named pipe
        create_pipe();

        // Wait for a client to connect
        wait_for_connection();

        // Start the connection check timer
        start_client_check_timer();
    }
    catch (const std::exception &e) {
        std::cerr << "Exception in start: " << e.what() << std::endl;
        stop();
    }
}

void NamedPipeServer::stop() {
    if (is_stopped_.exchange(true)) {
        return;
    }

    // Cancel the timer
    timer_.cancel();

    // Close the pipe if it's open
    close_pipe();
}

void NamedPipeServer::write(const std::string &message) {
    if (is_stopped_ || !is_connected_) {
        return;
    }

    boost::asio::post(strand_, [this, message]() {
        bool write_in_progress = !write_queue_.empty();
        write_queue_.push_back(message);

        if (!write_in_progress) {
            do_write();
        }
    });
}

bool NamedPipeServer::is_connected() const {
    return is_connected_ && !is_stopped_;
}

void NamedPipeServer::create_pipe() {
    // Create the named pipe
    HANDLE pipe_handle = CreateNamedPipeA(
            pipe_name_.c_str(),
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1,  // Only one instance since we're handling only one client
            4096, 4096, 0, nullptr);

    if (pipe_handle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("CreateNamedPipe failed: " + std::to_string(GetLastError()));
    }

    // Associate the pipe with Asio
    pipe_.assign(pipe_handle);
}

void NamedPipeServer::wait_for_connection() {
    // Create a Windows event for overlapped I/O
    OVERLAPPED *overlapped = new OVERLAPPED();
    ZeroMemory(overlapped, sizeof(OVERLAPPED));
    overlapped->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!overlapped->hEvent) {
        delete overlapped;
        throw std::runtime_error("Failed to create event: " + std::to_string(GetLastError()));
    }

    // Start the asynchronous ConnectNamedPipe operation
    BOOL result = ConnectNamedPipe(pipe_.native_handle(), overlapped);
    DWORD lastError = GetLastError();

    if (result || lastError == ERROR_PIPE_CONNECTED) {
        // Connection completed immediately
        CloseHandle(overlapped->hEvent);
        delete overlapped;

        // Client is connected
        is_connected_ = true;
        on_connect_();
        start_read();
    } else if (lastError == ERROR_IO_PENDING) {
        // Connection is pending - create an object_handle to wait for the event
        auto event_handle = std::make_shared<boost::asio::windows::object_handle>(
                io_context_, overlapped->hEvent);

        // Capture the overlapped pointer for proper cleanup
        event_handle->async_wait(
                boost::asio::bind_executor(strand_,
                                           [this, event_handle, overlapped](const error_code &ec) {
                                               // Clean up the event and overlapped structure
                                               CloseHandle(overlapped->hEvent);
                                               delete overlapped;

                                               if (ec || is_stopped_) {
                                                   close_pipe();
                                                   if (!is_stopped_) {
                                                       try {
                                                           create_pipe();
                                                           wait_for_connection();
                                                       } catch (const std::exception &e) {
                                                           std::cerr << "Error restarting connection: " << e.what()
                                                                     << std::endl;
                                                       }
                                                   }
                                                   return;
                                               }

                                               // Client is now connected
                                               is_connected_ = true;
                                               on_connect_();
                                               start_read();
                                           }
                )
        );
    } else {
        // Error occurred
        CloseHandle(overlapped->hEvent);
        delete overlapped;
        close_pipe();

        if (!is_stopped_) {
            try {
                // Try to create a new pipe and wait for connection again
                create_pipe();
                wait_for_connection();
            } catch (const std::exception &e) {
                std::cerr << "Error restarting connection: " << e.what() << std::endl;
            }
        }
    }
}

void NamedPipeServer::close_pipe() {
    if (pipe_.is_open()) {
        error_code ec;
        pipe_.close(ec);
    }

    if (is_connected_) {
        is_connected_ = false;
        on_disconnect_();
    }
}

void NamedPipeServer::start_read() {
    if (is_stopped_ || !is_connected_) {
        return;
    }

    buffer_.resize(4096);

    pipe_.async_read_some(
            boost::asio::buffer(buffer_),
            boost::asio::bind_executor(strand_,
                                       [this](const error_code &ec, std::size_t bytes_transferred) {
                                           if (!ec) {
                                               if (bytes_transferred > 0) {
                                                   // Process received data
                                                   std::string received_data(buffer_.begin(),
                                                                             buffer_.begin() + bytes_transferred);

                                                   // Call the message handler
                                                   on_message_(received_data);
                                               }

                                               // Continue reading
                                               start_read();
                                           } else {
                                               handle_error(ec);
                                           }
                                       }
            )
    );
}

void NamedPipeServer::do_write() {
    if (is_stopped_ || !is_connected_ || write_queue_.empty()) {
        return;
    }

    boost::asio::async_write(
            pipe_,
            boost::asio::buffer(write_queue_.front()),
            boost::asio::bind_executor(strand_,
                                       [this](const error_code &ec, std::size_t /*bytes_transferred*/) {
                                           if (!ec) {
                                               write_queue_.pop_front();

                                               if (!write_queue_.empty()) {
                                                   do_write();
                                               }
                                           } else {
                                               handle_error(ec);
                                           }
                                       }
            )
    );
}

void NamedPipeServer::handle_error(const error_code &ec) {
    // Check for client disconnect
    if (ec == boost::asio::error::eof ||
        ec == boost::asio::error::connection_reset ||
        ec == boost::asio::error::connection_aborted ||
        ec == boost::asio::error::broken_pipe ||
        ec == boost::asio::error::bad_descriptor) {

        close_pipe();

        // If not stopped, try to restart the server
        if (!is_stopped_) {
            try {
                create_pipe();
                wait_for_connection();
            }
            catch (const std::exception &e) {
                std::cerr << "Error restarting server: " << e.what() << std::endl;
            }
        }
    } else {
        // Other errors
        on_error_(ec);
    }
}

void NamedPipeServer::start_client_check_timer() {
    if (is_stopped_) return;

    // Schedule the timer
    timer_.expires_after(std::chrono::seconds(5));
    timer_.async_wait([this](const error_code &ec) {
        if (!ec && !is_stopped_) {
            check_client_connection();
            // Reschedule the timer
            start_client_check_timer();
        }
    });
}

void NamedPipeServer::check_client_connection() {
    if (!is_connected_ || is_stopped_) {
        return;
    }

    // Check if client is still connected
    if (!check_pipe_connected()) {
        close_pipe();

        // Try to restart the server
        try {
            create_pipe();
            wait_for_connection();
        }
        catch (const std::exception &e) {
            std::cerr << "Error restarting server: " << e.what() << std::endl;
        }
    }
}

bool NamedPipeServer::check_pipe_connected() {
    if (!pipe_.is_open()) {
        return false;
    }

    // Peek at the pipe to see if it's still connected
    DWORD bytes_available = 0;
    BOOL result = PeekNamedPipe(
            pipe_.native_handle(),
            nullptr,
            0,
            nullptr,
            &bytes_available,
            nullptr
    );

    if (!result) {
        DWORD last_error = GetLastError();
        // ERROR_BROKEN_PIPE or ERROR_BAD_PIPE indicates the client has disconnected
        return !(last_error == ERROR_BROKEN_PIPE || last_error == ERROR_BAD_PIPE ||
                 last_error == ERROR_PIPE_NOT_CONNECTED || last_error == ERROR_NO_DATA);
    }

    return true;
}
