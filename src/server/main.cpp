#include <iostream>
#include <map>
#include <thread>
#include <pthread.h>

#include "../socket/socket.hpp"
#include "../lib/discovery.hpp"
#include "../lib/processing.hpp"
#include "../lib/client_map.hpp"
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

    ClientMap::ClientMap client_map;

    Discovery::DiscoveryServiceServer discovery_service(socket_instance, client_map);
    Processing::ProcessingServiceServer processing_service(socket_instance, client_map);

    discovery_service.find_primary_server();

    logger.server_hello();

    while (true)
    {
        if (!discovery_service.is_primary_server())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        auto message = socket_instance.receive();

        if (!message.is_valid)
            continue;

        auto data = message.data;
        const std::string client_ip = inet_ntoa(message.sender_addr.sin_addr);

        // Callback para processar mensagens recebidas em uma thread separada
        auto process_message = [&logger, &discovery_service, &processing_service, &client_map, data, client_ip]()
        {
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

                processing_service.process_request(client_ip, request_id, number);
                return;
            }

            if (discovery_service.is_keep_alive_message(data))
            {
                logger.debug("Keep alive message received");
                discovery_service.im_alive(client_ip);
                return;
            }

            if (discovery_service.is_server_discovery_message(data))
            {
                discovery_service.process_server_discovery(client_ip);
                return;
            }

            if (discovery_service.is_client_discovery_message(data))
            {
                discovery_service.respond(client_ip);
                client_map.add_client(client_ip);
                return;
            }

            if (processing_service.is_exit_message(data))
            {
                processing_service.handle_exit_message(client_ip);
                return;
            }
        };

        // Cria uma thread para processar a mensagem
        std::thread(process_message).detach();
    }

    return 0;
}
