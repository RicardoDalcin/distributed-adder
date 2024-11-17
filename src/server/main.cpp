#include <iostream>
#include <map>
#include <thread>

#include "../socket/socket.hpp"
#include "../lib/discovery.hpp"
#include "../logger/logger.hpp"

struct ClientEntry
{
    std::string address;
    int last_req;
    int last_sum;
};

int main(int argc, char *argv[])
{
    Logger::Logger logger;
    Discovery::DiscoveryService discovery_service;
    std::map<std::string, ClientEntry> client_map;

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

    auto on_receive = [&logger, &socket_instance, &discovery_service, &client_map](const std::string &data, const struct sockaddr_in &sender_addr)
    {
        std::thread([&logger, &socket_instance, &discovery_service, &client_map, &data, &sender_addr]()
                    {
            if (discovery_service.is_discovery_message(data))
            {
                discovery_service.respond(socket_instance, sender_addr);
                std::string client_ip = inet_ntoa(sender_addr.sin_addr);

                ClientEntry new_client;
                new_client.address = client_ip;
                new_client.last_req = 0;
                new_client.last_sum = 0;

                client_map.insert(std::pair<std::string, ClientEntry>(client_ip, new_client));
                return;
            }

            std::string request_id = data.substr(0, data.find(";"));
            std::string number = data.substr(data.find(";") + 1);
            int input_number = std::stoi(number);

            logger.log("Msg #" + request_id + " from " + inet_ntoa(sender_addr.sin_addr) + ": " + std::to_string(input_number)); })
            .detach();
    };

    socket_instance.receive_callback(on_receive);

    return 0;
}
