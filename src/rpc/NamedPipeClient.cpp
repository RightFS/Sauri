//
// Created by Right on 25/5/19 星期一 17:39.
//
#include "NamedPipeClient.h"

#include <utility>
#include "logger_helper.h"


NamedPipeClient::NamedPipeClient(asio::io_context &io_context, std::string server_name)
        : io_context_(io_context),
          server_name_(std::move(server_name)),
          pipe_(nullptr),
          buffer_(4096),
          connected_(false) {
}

bool NamedPipeClient::connect() {
    if (connected_) return true;

    std::string pipe_name = "\\\\.\\pipe\\" + server_name_;

    // 尝试连接到命名管道
    HANDLE pipe_handle = CreateFileA(
            pipe_name.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0, NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL
    );

    if (pipe_handle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error == ERROR_PIPE_BUSY) {
            std::cerr << "管道忙，等待连接..." << std::endl;

            // 等待管道可用
            if (!WaitNamedPipeA(pipe_name.c_str(), 5000)) {
                std::cerr << "等待管道超时，错误码: " << GetLastError() << std::endl;
                return false;
            }

            // 再次尝试连接
            pipe_handle = CreateFileA(
                    pipe_name.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    0, NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_OVERLAPPED,
                    NULL
            );

            if (pipe_handle == INVALID_HANDLE_VALUE) {
                std::cerr << "连接到命名管道失败，错误码: " << GetLastError() << std::endl;
                return false;
            }
        } else {
            std::cerr << "连接到命名管道失败，错误码: " << error << std::endl;
            return false;
        }
    }

    // 创建 stream_handle
    pipe_ = std::make_unique<asio::windows::stream_handle>(io_context_, pipe_handle);
    connected_ = true;

    // 通知连接成功
    if (connection_handler_) {
        connection_handler_(true);
    }

    // 开始读取
    start_read();

    return true;
}

void NamedPipeClient::send(const std::string &message) {
    if (!connected_ || !pipe_) return;

    auto self = shared_from_this();

    // 使用strand确保写入操作的顺序
    asio::post(strand_, [this, self, message]() {
        bool write_in_progress = !write_queue_.empty();
        write_queue_.push_back(message);

        if (!write_in_progress) {
            do_write();
        }
    });
}

void NamedPipeClient::set_message_handler(NamedPipeClient::MessageHandler handler) {
    message_handler_ = handler;
}

void NamedPipeClient::set_connection_handler(NamedPipeClient::ConnectionHandler handler) {
    connection_handler_ = handler;
}

void NamedPipeClient::close() {
    if (!connected_) return;

    connected_ = false;
    if (pipe_) {
        pipe_->close();
        pipe_.reset();
    }

    // 通知断开连接
    if (connection_handler_) {
        connection_handler_(false);
    }
}

bool NamedPipeClient::is_connected() const {
    return connected_;
}

void NamedPipeClient::start_read() {
    if (!connected_ || !pipe_) return;

    auto self = shared_from_this();
    pipe_->async_read_some(asio::buffer(buffer_),
                           [this, self](const boost::system::error_code &ec, std::size_t bytes_transferred) {
                               handle_read(ec, bytes_transferred);
                           });
}

void NamedPipeClient::handle_read(const boost::system::error_code &ec, std::size_t bytes_transferred) {
    if (!ec) {
        // 处理接收到的消息
        std::string message(buffer_.data(), bytes_transferred);

        // 调用消息处理回调
        if (message_handler_) {
            message_handler_(message);
        }

        // 继续读取
        start_read();
    } else {
        // 连接断开或错误
        close();
    }
}

void NamedPipeClient::do_write() {
    if (write_queue_.empty() || !connected_ || !pipe_) {
        return;
    }

    auto self = shared_from_this();
    asio::async_write(*pipe_, asio::buffer(write_queue_.front()),
                      asio::bind_executor(strand_, [this, self](const boost::system::error_code &ec,
                                                                std::size_t /*bytes_transferred*/) {
                          if (!ec) {
                              write_queue_.pop_front();

                              if (!write_queue_.empty()) {
                                  do_write();
                              }
                          } else {
                              // 写入错误，连接可能已断开
                              close();
                          }
                      }));
}

