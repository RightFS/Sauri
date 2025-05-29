#pragma once


#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include <deque>
#include "boost/asio.hpp"
#include "boost/bind/bind.hpp"

using boost::asio::windows::stream_handle;

// Define callback function types
using message_handler = std::function<void(const std::string &)>;
using error_handler = std::function<void(const boost::system::error_code &)>;
using connect_handler = std::function<void()>;
using disconnect_handler = std::function<void()>;

class NamedPipeServer {
public:
    NamedPipeServer(boost::asio::io_context &io_context, const std::string &pipe_name);

    ~NamedPipeServer();

    // Set handler for incoming messages
    void set_message_handler(message_handler handler);

    // Set handler for errors
    void set_error_handler(error_handler handler);

    // Set handler for client connection
    void set_connect_handler(connect_handler handler);

    // Set handler for client disconnection
    void set_disconnect_handler(disconnect_handler handler);

    void start();

    void stop();

    // Write payload to the connected client
    void write(const std::string &message);

    // Check if a client is connected
    bool is_connected() const;

private:
    void create_pipe();

    void wait_for_connection();

    void close_pipe();

    void start_read();

    void do_write();

    void handle_error(const boost::system::error_code &ec);

    void start_client_check_timer();

    void check_client_connection();

    bool check_pipe_connected();

    boost::asio::io_context &io_context_;
    std::string pipe_name_;
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    stream_handle pipe_;
    boost::asio::steady_timer timer_;
    std::vector<char> buffer_;
    std::deque<std::string> write_queue_;
    std::atomic<bool> is_connected_;
    std::atomic<bool> is_stopped_;
    message_handler on_message_;
    error_handler on_error_;
    connect_handler on_connect_;
    disconnect_handler on_disconnect_;
};
