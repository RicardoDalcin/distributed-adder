#include <iostream>
#include "../socket/socket.hpp"
#include "../logger/logger.hpp"
#include "../lib/discovery.hpp"
#include "../lib/processing.hpp"
#include "../lib/utils.hpp"

int main(int argc, char *argv[])
{
    Logger::Logger logger;
    Discovery::DiscoveryService discovery_service;
    Processing::ProcessingService processing_service;

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

    std::string server_ip = discovery_service.find_server_ip(socket_instance);

    if (server_ip.empty())
    {
        logger.error("Discovery failed. No server IP address found.");
        return 1;
    }

    logger.client_hello(server_ip);
    socket_instance.set_server_ip(server_ip);

    std::string line;
    int sum = 0;
    while (std::getline(std::cin, line))
    {
        try
        {
            int input_number = std::stoi(line);
            sum += input_number;
            processing_service.send_request(socket_instance, input_number);
        }
        catch (std::invalid_argument &e)
        {
            logger.error("Invalid number: " + line);
        }

        logger.log("Sum: " + std::to_string(sum));
    }

    return 0;
}
