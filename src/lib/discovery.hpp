#ifndef DISCOVERY_HPP
#define DISCOVERY_HPP

#include <map>
#include <string>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <thread>
#include <chrono>

#include "../logger/logger.hpp"
#include "../socket/socket.hpp"
#include "../lib/client_map.hpp"
#include "../lib/server_map.hpp"
#include "../lib/election.hpp"

namespace Discovery
{
    const std::string CLIENT_DISCOVERY_MESSAGE = "CLIENT_DISCOVERY";
    const std::string SERVER_DISCOVERY_MESSAGE = "SERVER_DISCOVERY";

    const std::string KEEP_ALIVE_MESSAGE = "KEEP_ALIVE";
    const std::string IM_ALIVE_MESSAGE = "IM_ALIVE";

    const std::string DISCOVERY_RESPONSE = "DISCOVERY_RESPONSE";
    const std::string UPDATE_SERVER_LIST_MESSAGE = "UPDATE_SERVER_LIST";

    enum class ServerType
    {
        Unknown,
        Primary,
        Replica
    };

    class DiscoveryServiceServer
    {
    private:
        const int REQUEST_TIMEOUT_MS = 30;

        ServerType server_type = ServerType::Unknown;

        Logger::Logger logger;

        SocketInstance::SocketInstance &socket_instance;
        ClientMap::ClientMap &client_map;
        ServerMap::ServerMap &server_map;
        Election::Election &election;
        std::string primary_server_ip;

    public:
        DiscoveryServiceServer(SocketInstance::SocketInstance &socket_instance, ClientMap::ClientMap &client_map, ServerMap::ServerMap &server_map, Election::Election &election)
            : socket_instance(socket_instance),
              client_map(client_map),
              server_map(server_map),
              election(election)
        {
        }

        ~DiscoveryServiceServer() {}

        ServerMap::ServerMap &get_server_map()
        {
            return server_map;
        }

        bool is_primary_server()
        {
            return server_type == ServerType::Primary;
        }

        void find_primary_server()
        {
            // Envia a mensagem de descoberta até 3 vezes
            // Caso ocorra timeout nas três tentativas, considera o servidor como primário
            int tries = 0;
            ServerType new_server_type = ServerType::Primary;
            std::string new_server_ip = "";

            while (tries < 3 && new_server_type == ServerType::Primary)
            {
                tries++;
                logger.debug("Sending discovery message tries: " + std::to_string(tries));

                socket_instance.send_broadcast(SERVER_DISCOVERY_MESSAGE);
                auto message = socket_instance.wait_for_message(DISCOVERY_RESPONSE, REQUEST_TIMEOUT_MS);

                if (message.result == SocketInstance::WaitMessageResult::Success)
                {
                    std::string ip = inet_ntoa(message.message.sender_addr.sin_addr);
                    new_server_ip = ip;
                    new_server_type = ServerType::Replica;

                    auto msg = Utils::parse_message(message.message.data);

                    if (msg.fields_count < 2)
                    {
                        logger.error("Invalid server discovery message: " + message.message.data);
                        continue;
                    }

                    int server_id = std::stoi(msg.fields[1]);
                    server_map.set_id(server_id);
                    server_map.load_from_string(msg.fields[2]);

                    logger.debug("Server id: " + std::to_string(server_id));
                    logger.debug("Server map: " + server_map.to_string());

                    continue;
                }
            }

            logger.debug("Server type: " + std::to_string((int)new_server_type));

            server_type = new_server_type;
            primary_server_ip = new_server_ip;

            if (server_type == ServerType::Replica)
            {
                socket_instance.set_server_ip(primary_server_ip);
            }
        }

        void set_self_primary_server()
        {
            server_type = ServerType::Primary;
            primary_server_ip = "";
        }

        void set_primary_server(std::string ip)
        {
            server_type = ServerType::Replica;
            primary_server_ip = ip;
            socket_instance.set_server_ip(ip);
        }

        bool is_im_alive_message(std::string message)
        {
            return message == IM_ALIVE_MESSAGE;
        }

        bool is_keep_alive_message(std::string message)
        {
            return message == KEEP_ALIVE_MESSAGE;
        }

        bool is_client_discovery_message(std::string message)
        {
            return message == CLIENT_DISCOVERY_MESSAGE;
        }

        bool is_server_discovery_message(std::string message)
        {
            return message == SERVER_DISCOVERY_MESSAGE;
        }

        void im_alive(std::string client_ip)
        {
            // logger.debug("Sending im alive message to " + client_ip);
            socket_instance.send_to_ip(IM_ALIVE_MESSAGE, client_ip);
        }

        void respond(std::string client_ip)
        {
            socket_instance.send_to_ip(DISCOVERY_RESPONSE, client_ip);
        }

        void respond_server(std::string client_ip, int server_id)
        {
            std::string message = DISCOVERY_RESPONSE + Utils::DELIMITER + std::to_string(server_id) + Utils::DELIMITER + server_map.to_string();
            socket_instance.send_to_ip(message, client_ip);
        }

        bool is_update_server_list_message(std::string message)
        {
            return Utils::starts_with(message, UPDATE_SERVER_LIST_MESSAGE + Utils::DELIMITER);
        }

        void process_server_discovery(std::string client_ip)
        {
            logger.debug("Processing server discovery");
            int id = server_map.add_server(client_ip);

            logger.debug("Added server " + client_ip + " with id " + std::to_string(id));
            update_server_list();
            respond_server(client_ip, id);
        }

        void update_server_list()
        {
            logger.debug("Updating server list");
            std::string message = UPDATE_SERVER_LIST_MESSAGE + Utils::DELIMITER + server_map.to_string();
            server_map.iterate([this, message](std::pair<std::string, int> data)
                               { socket_instance.send_to_ip(message, data.first); });
        }

        void handle_update_server_list_message(std::string message)
        {
            std::string str_server_map = Utils::parse_message(message).fields[1];
            server_map.load_from_string(str_server_map);
            logger.debug("Server map updated: " + server_map.to_string());
        }
    };

    class DiscoveryServiceClient
    {
    private:
        Logger::Logger logger;
        SocketInstance::SocketInstance &socket_instance;

    public:
        DiscoveryServiceClient(SocketInstance::SocketInstance &socket_instance)
            : socket_instance(socket_instance) {}

        ~DiscoveryServiceClient() {}

        std::string find_server_ip()
        {
            std::string server_ip = "";
            bool wait_for_response = true;

            logger.debug("Waiting for server ip");

            while (wait_for_response)
            {
                // Envia a mensagem de descoberta em broadcast
                socket_instance.send_broadcast(CLIENT_DISCOVERY_MESSAGE);

                // Espera a resposta do servidor
                auto message = socket_instance.receive();

                // Verifica se recebeu uma resposta de descoberta
                if (message.is_valid && message.data == DISCOVERY_RESPONSE)
                {
                    wait_for_response = false;
                    server_ip = inet_ntoa(message.sender_addr.sin_addr);
                }
            }

            logger.debug("Server ip found: " + server_ip);

            return server_ip;
        }
    };
}

#endif // DISCOVERY_HPP