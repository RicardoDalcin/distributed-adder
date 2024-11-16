#include <iostream>
#include "../socket/socket.hpp"
#include "../logger/logger.hpp"

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

    logger.server_hello();

    auto on_receive = [&logger, &socket_instance](const std::string &data, const struct sockaddr_in &sender_addr)
    {
        socket_instance.send("ping back", sender_addr);
    };

    socket_instance.receiveCallback(on_receive);

    return 0;
}
