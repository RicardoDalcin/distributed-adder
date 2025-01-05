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

namespace Discovery
{
    const std::string CLIENT_DISCOVERY_MESSAGE = "CLIENT_DISCOVERY";
    const std::string SERVER_DISCOVERY_MESSAGE = "SERVER_DISCOVERY";

    const std::string KEEP_ALIVE_MESSAGE = "KEEP_ALIVE";
    const std::string IM_ALIVE_MESSAGE = "IM_ALIVE";

    const std::string DISCOVERY_RESPONSE = "DISCOVERY_RESPONSE";

    const std::string ELECTION_START_MESSAGE = "ELECTION";
    const std::string ELECTION_ANSWER_MESSAGE = "ELECTION_ANSWER";
    const std::string ELECTION_COORDINATOR_MESSAGE = "ELECTION_COORDINATOR";

    enum class ServerType
    {
        Unknown,
        Primary,
        Replica
    };

    class DiscoveryServiceServer
    {
    private:
        const int REQUEST_TIMEOUT_MS = 10;

        ServerType server_type = ServerType::Unknown;

        Logger::Logger logger;

        SocketInstance::SocketInstance &socket_instance;
        ClientMap::ClientMap &client_map;
        ServerMap::ServerMap &server_map;
        std::string primary_server_ip;

    public:
        DiscoveryServiceServer(SocketInstance::SocketInstance &socket_instance, ClientMap::ClientMap &client_map, ServerMap::ServerMap &server_map)
            : socket_instance(socket_instance),
              client_map(client_map),
              server_map(server_map)
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
                // keep_alive();
            }
        }

        void keep_alive()
        {
            auto keep_alive_thread = [this]()
            {
                bool server_alive = true;
                socket_instance.set_timeout(REQUEST_TIMEOUT_MS);

                while (server_alive)
                {
                    socket_instance.send_to_server(KEEP_ALIVE_MESSAGE);
                    auto message = socket_instance.wait_for_message(IM_ALIVE_MESSAGE, REQUEST_TIMEOUT_MS);

                    if (message.result == SocketInstance::WaitMessageResult::Timeout)
                    {
                        server_alive = false;
                        continue;
                    }

                    // Espera 500ms
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }

                logger.error("SERVER IS DEAD");
            };

            std::thread(keep_alive_thread).detach();
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
            logger.debug("Sending im alive message to " + client_ip);
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

        void process_server_discovery(std::string client_ip)
        {
            logger.debug("Processing server discovery");
            int id = server_map.add_server(client_ip);
            logger.debug("Added server " + client_ip + " with id " + std::to_string(id));
            respond_server(client_ip, id);
        }
    };

    class DiscoveryServiceClient
    {
    private:
        SocketInstance::SocketInstance &socket_instance;

    public:
        DiscoveryServiceClient(SocketInstance::SocketInstance &socket_instance)
            : socket_instance(socket_instance) {}

        ~DiscoveryServiceClient() {}

        std::string find_server_ip()
        {
            std::string server_ip = "";
            bool wait_for_response = true;

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

            return server_ip;
        }
    };
}

#endif // DISCOVERY_HPP