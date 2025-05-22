#include "boost/asio.hpp"
#include <iostream>
#include <string>
#include <memory>
#include <Windows.h>
#include <deque>

namespace asio = boost::asio;

class NamedPipeClient : public std::enable_shared_from_this<NamedPipeClient> {
public:
    using MessageHandler = std::function<void(const std::string&)>;
    using ConnectionHandler = std::function<void(bool)>;

    NamedPipeClient(asio::io_context& io_context, std::string  server_name);

    // 连接到服务器
    bool connect();

    // 发送消息
    void write(const std::string& message);

    // 设置消息处理回调
    void set_message_handler(MessageHandler handler);

    // 设置连接状态回调
    void set_connection_handler(ConnectionHandler handler);

    // 关闭连接
    void close();

    // 检查是否已连接
    bool is_connected() const;

private:
    // 开始异步读取
    void start_read();

    // 处理读取完成
    void handle_read(const boost::system::error_code& ec, std::size_t bytes_transferred);

    // 执行写入操作
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