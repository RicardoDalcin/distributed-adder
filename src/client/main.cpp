#include <iostream>
#include "../socket/socket.hpp"
#include "../logger/logger.hpp"
#include "../lib/utils.hpp"

int main(int argc, char *argv[])
{
    Logger::Logger logger;

    if (argc < 2)
    {
        logger.error("Missing required argument: port");
        return 1;
    }

    int port = atoi(argv[1]);

    if (port < 1 || port > 65535)
    {
        logger.error("Invalid port number: " + std::to_string(port));
        return 1;
    }

    auto socket_instance = SocketInstance::SocketInstance(port);
    auto init_result = socket_instance.init();

    if (init_result != SocketInstance::SocketInitResult::Success)
    {
        logger.socket_init_error(init_result);
        return 1;
    }

    std::string client_ip = Utils::get_local_ip_address();
    std::string server_ip = "";

    socket_instance.send_broadcast("Hello, broadcast!");

    bool wait_for_response = true;
    while (wait_for_response)
    {
        auto message = socket_instance.receive();

        if (message.is_valid && inet_ntoa(message.sender_addr.sin_addr) != client_ip)
        {
            wait_for_response = false;
            server_ip = inet_ntoa(message.sender_addr.sin_addr);
        }
    }

    logger.client_hello(server_ip);

    std::string line;
    while (std::getline(std::cin, line))
    {
        try
        {
            int input_number = std::stoi(line);
            logger.log("Sent message: " + std::to_string(input_number));
        }
        catch (std::invalid_argument &e)
        {
            logger.error("Invalid number: " + line);
        }
    }

    return 0;
}
