#include <iostream>
#include <map>
#include <thread>
#include <pthread.h>

#include "../socket/socket.hpp"
#include "../lib/discovery.hpp"
#include "../lib/processing.hpp"
#include "../logger/logger.hpp"

struct ClientEntry
{
    std::string address;
    int last_req;
    int last_sum;
} typedef client_entry;

int main(int argc, char *argv[])
{
    Logger::Logger logger;
    Discovery::DiscoveryService discovery_service;
    Processing::ProcessingService processing_service;
    std::map<std::string, client_entry> client_map;

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

    int shared_sum = 0;

    while (true)
    {
        auto message = socket_instance.receive();

        if (!message.is_valid)
            continue;

        auto data = message.data;
        auto sender_addr = message.sender_addr;

        auto process_message = [&logger, &shared_sum, &socket_instance, &discovery_service, &processing_service, &client_map, data, sender_addr]()
        {
            if (discovery_service.is_discovery_message(data))
            {
                discovery_service.respond(socket_instance, sender_addr);
                std::string client_ip = inet_ntoa(sender_addr.sin_addr);

                client_entry new_client;
                new_client.address = client_ip;
                new_client.last_req = 0;
                new_client.last_sum = 0;

                client_map.insert(std::pair<std::string, client_entry>(client_ip, new_client));
                return;
            }

            if (processing_service.is_request_message(data))
            {
                auto params = Utils::parse_message(data);

                if (params.fields_count < 3)
                {
                    logger.error("Invalid request message: " + data);
                    return;
                }

                int request_id = std::stoi(params.fields[1]);
                int number = std::stoi(params.fields[2]);

                processing_service.process_request(socket_instance, sender_addr, request_id, number);
                return;
            }
        };

        std::thread(process_message).detach();
    }

    return 0;
}
