#include <opencv2/opencv.hpp>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <boost/asio.hpp>

std::mutex frame_mutex;        // 线程锁，用于保护帧数据
cv::Mat current_frame;         // 当前帧数据
cv::Size resize_dim(640, 480); // 缩放后的图像尺寸

/**
 * @brief 捕获RTSP流并缩放帧尺寸
 *
 * @param rtsp_url RTSP流地址
 */
void captureRTSPStream(const std::string &rtsp_url)
{
    cv::VideoCapture cap(rtsp_url);
    if (!cap.isOpened())
    {
        std::cerr << "Error: Cannot open RTSP stream" << std::endl;
        return;
    }

    while (true)
    {
        cv::Mat frame;
        cap >> frame; // 从RTSP流中读取一帧
        if (frame.empty())
            continue;

        // 缩放图像尺寸以减少传输数据量
        cv::Mat resized_frame;
        cv::resize(frame, resized_frame, resize_dim);

        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            current_frame = resized_frame.clone();
        }
    }
}

/**
 * @brief 处理客户端请求，发送视频流
 *
 * @param socket 连接的客户端socket
 */
void handleClient(boost::asio::ip::tcp::socket socket)
{
    try
    {
        // HTTP响应头，设置Content-Type为multipart/x-mixed-replace
        std::ostringstream header;
        header << "HTTP/1.1 200 OK\r\n"
               << "Content-Type: multipart/x-mixed-replace;boundary=frame\r\n\r\n";
        boost::asio::write(socket, boost::asio::buffer(header.str()));

        while (true)
        {
            std::vector<uchar> buffer; // JPEG编码后的帧数据
            {
                std::lock_guard<std::mutex> lock(frame_mutex);
                if (current_frame.empty())
                    continue;

                // 将当前帧编码为JPEG格式
                cv::imencode(".jpg", current_frame, buffer);
            }

            // 发送HTTP分隔符和帧信息
            std::ostringstream frame_data;
            frame_data << "--frame\r\n"
                       << "Content-Type: image/jpeg\r\n"
                       << "Content-Length: " << buffer.size() << "\r\n\r\n";

            boost::asio::write(socket, boost::asio::buffer(frame_data.str()));
            boost::asio::write(socket, boost::asio::buffer(buffer.data(), buffer.size()));
            boost::asio::write(socket, boost::asio::buffer("\r\n"));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Client disconnected: " << e.what() << std::endl;
    }
}

/**
 * @brief 启动HTTP服务器，监听客户端连接
 *
 * @param port 监听端口
 */
void startHTTPServer(unsigned short port)
{
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor(io_context, {boost::asio::ip::tcp::v4(), port});

    std::cout << "Server started on port " << port << std::endl;

    while (true)
    {
        boost::asio::ip::tcp::socket socket(io_context);
        acceptor.accept(socket);
        socket.set_option(boost::asio::ip::tcp::no_delay(true));
        std::cout << "New client connected" << std::endl;

        // 为每个客户端启动一个独立线程处理
        std::thread(handleClient, std::move(socket)).detach();
    }
}

int main()
{
    // RTSP流地址和HTTP服务器端口
    std::string rtsp_url = "rtsp://admin:taili2408@192.168.123.123:554/Streaming/Channels/101";
    unsigned short port = 8080;

    // 启动捕获RTSP流线程
    std::thread capture_thread(captureRTSPStream, rtsp_url);

    // 启动HTTP服务器线程
    std::thread server_thread(startHTTPServer, port);

    capture_thread.join();
    server_thread.join();

    return 0;
}