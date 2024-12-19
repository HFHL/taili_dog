#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <unitree/robot/go2/sport/sport_client.hpp>
#include <unitree/robot/channel/channel_factory.hpp>

// Global flag to stop the server gracefully
static bool g_running = true;

void signalHandler(int signum)
{
    g_running = false;
}


int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " <networkInterface> <port>" << std::endl;
        return -1;
    }

    std::string networkInterface = argv[1];
    int port = std::stoi(argv[2]);

    // Initialize the SDK communications for the robot
    // This must be done before creating the SportClient
    unitree::robot::ChannelFactory::Instance()->Init(0, networkInterface);
    unitree::robot::go2::SportClient sport_client;
    sport_client.SetTimeout(10.0f);

    sport_client.Init();
    

    // Create a listening TCP socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        std::cerr << "Failed to create socket." << std::endl;
        return -1;
    }

    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "Failed to bind." << std::endl;
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 5) < 0)
    {
        std::cerr << "Failed to listen." << std::endl;
        close(server_fd);
        return -1;
    }

    std::cout << "Server listening on port " << port << ". Waiting for commands..." << std::endl;

    // Set up signal handler for graceful shutdown
    signal(SIGINT, signalHandler);

    while (g_running)
    {
        // Accept a client connection (blocking)
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            if (g_running)
            {
                std::cerr << "Failed to accept connection." << std::endl;
            }
            continue;
        }

        std::cout << "Client connected." << std::endl;

        // Simple loop to read commands line by line
        char buffer[1024];
        std::memset(buffer, 0, sizeof(buffer));

        while (g_running)
        {
            ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0)
            {
                std::cout << "Client disconnected." << std::endl;
                close(client_fd);
                break;
            }

            buffer[bytes_read] = '\0';
            std::string command(buffer);
            // Trim command (remove newline)
            if (!command.empty() && command.back() == '\n')
            {
                command.pop_back();
            }
            if (!command.empty() && command.back() == '\r')
            {
                command.pop_back();
            }

            std::cout << "Received command: " << command << std::endl;

            // Execute corresponding sport_client command
            if (command == "stand_up")
            {
                int32_t ret = sport_client.StandUp();
                ssize_t bytes_written = write(client_fd, (ret == 0 ? "OK\n" : "ERR\n"), 4);
            }
            else if (command == "sit_down")
            {
                // The given API has a Sit() and StandDown() method.
                // If you want a "sit" position (if available in newer releases), use it directly.
                // Otherwise, using StandDown() to represent a low stance:
                int32_t ret = sport_client.StandDown();
                ssize_t bytes_written = write(client_fd, (ret == 0 ? "OK\n" : "ERR\n"), 4);
            }
            else if (command == "forward")
            {
                // Move forward at a given speed. Adjust vx if you need.
                sport_client.BalanceStand();
                int32_t ret = sport_client.Move(0.3, 0, 0.0);
                // print int32_t ret
                std::cout << ret << std::endl;
                ssize_t bytes_written = write(client_fd, (ret == 0 ? "OK\n" : "ERR\n"), 4);
            }
            else if (command == "backward")
            {   
                sport_client.BalanceStand();
                int32_t ret = sport_client.Move(-0.3, 0, 0.0);
                std::cout << ret << std::endl;
                ssize_t bytes_written = write(client_fd, (ret == 0 ? "OK\n" : "ERR\n"), 4);
            }
            else if (command == "left")
            {   
                sport_client.BalanceStand();
                int32_t ret = sport_client.Move(0.0, 0.2, 0.0);
                ssize_t bytes_written = write(client_fd, (ret == 0 ? "OK\n" : "ERR\n"), 4);
            }
            else if (command == "right")
            {   
                sport_client.BalanceStand();
                int32_t ret = sport_client.Move(0.0, -0.2, 0.0);
                ssize_t bytes_written = write(client_fd, (ret == 0 ? "OK\n" : "ERR\n"), 4);
            }
            else if (command == "turn_left")
            {   
                sport_client.BalanceStand();
                int32_t ret = sport_client.Move(0.1, 0.0, 0.5);
                ssize_t bytes_written = write(client_fd, (ret == 0 ? "OK\n" : "ERR\n"), 4);
            }
            else if (command == "turn_right")
            {   
                sport_client.BalanceStand();
                int32_t ret = sport_client.Move(0.1, 0.0, -0.5);
                ssize_t bytes_written = write(client_fd, (ret == 0 ? "OK\n" : "ERR\n"), 4);
            }
            else if (command == "stop")
            {   
                int32_t ret = sport_client.StopMove();
                ssize_t bytes_written = write(client_fd, (ret == 0 ? "OK\n" : "ERR\n"), 4);
            }
            else if (command == "stop")
            {
                int32_t ret = sport_client.StopMove();
 
                ssize_t bytes_written = write(client_fd, (ret == 0 ? "OK\n" : "ERR\n"), 4);
                
    
            }
            else if (command == "quit")
            {   
                ssize_t bytes_written = write(client_fd, "OK\n", 3);
                close(client_fd);
                break;
            }
            else
            {
                // Unknown command
                ssize_t bytes_written = write(client_fd, "UNKNOWN COMMAND\n", 16);
            }

            

            std::memset(buffer, 0, sizeof(buffer));
        }
    }

    close(server_fd);

    return 0;
}