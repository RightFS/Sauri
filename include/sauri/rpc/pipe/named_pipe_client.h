#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <deque>
#include "boost/asio.hpp"

namespace asio = boost::asio;

class NamedPipeClient : public std::enable_shared_from_this<NamedPipeClient> {
public:
    using MessageHandler = std::function<void(const std::string&)>;
    using ConnectionHandler = std::function<void(bool)>;

    NamedPipeClient(asio::io_context& io_context, std::string  server_name);

    // connect to the named pipe server
    bool connect();

    // send a message to the server
    void write(const std::string& message);

    // set message handler
    void set_message_handler(MessageHandler handler);

    void set_connection_handler(ConnectionHandler handler);

    void close();

    bool is_connected() const;

private:
    void start_read();

    void handle_read(const boost::system::error_code& ec, std::size_t bytes_transferred);

    void do_write();

    asio::io_context& io_context_;
    std::string server_name_;
    std::unique_ptr<asio::windows::stream_handle> pipe_;
    std::vector<char> buffer_;
    bool connected_;

    asio::io_context::strand strand_{io_context_};
    std::deque<std::string> write_queue_{};

    MessageHandler message_handler_;
    ConnectionHandler connection_handler_;
};