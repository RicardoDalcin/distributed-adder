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

namespace Discovery
{
    const std::string CLIENT_DISCOVERY_MESSAGE = "CLIENT_DISCOVERY";
    const std::string SERVER_DISCOVERY_MESSAGE = "SERVER_DISCOVERY";

    const std::string KEEP_ALIVE_MESSAGE = "KEEP_ALIVE";
    const std::string IM_ALIVE_MESSAGE = "IM_ALIVE";

    const std::string DISCOVERY_RESPONSE = "DISCOVERY_RESPONSE";

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
        std::map<std::string, bool> server_map;
        std::string primary_server_ip;

    public:
        DiscoveryServiceServer(SocketInstance::SocketInstance &socket_instance, ClientMap::ClientMap &client_map)
            : socket_instance(socket_instance),
              client_map(client_map)
        {
        }

        ~DiscoveryServiceServer() {}

        bool is_primary_server()
        {
            return server_type == ServerType::Primary;
        }

        void find_primary_server()
        {
            // Envia a mensagem de descoberta até 3 vezes
            // Caso ocorra timeout nas três tentativas, considerar o servidor primário
            int tries = 0;
            ServerType new_server_type = ServerType::Primary;
            std::string new_server_ip = "";

            while (tries < 3 && new_server_type == ServerType::Primary)
            {
                tries++;
                logger.debug("Sending discovery message tries: " + std::to_string(tries));

                bool wait_for_response = true;
                bool timed_out = false;

                auto start_time = std::chrono::system_clock::now();

                while (wait_for_response && !timed_out)
                {
                    // Envia a mensagem de descoberta em broadcast
                    socket_instance.send_broadcast(SERVER_DISCOVERY_MESSAGE);

                    // Espera a resposta do servidor
                    auto message = socket_instance.receive();

                    if (!message.is_valid)
                    {
                        // Em caso de timeout, passa para a próxima tentativa
                        wait_for_response = false;
                        continue;
                    }

                    // Verifica se recebeu uma resposta de descoberta
                    if (message.is_valid && message.data == DISCOVERY_RESPONSE)
                    {
                        std::string ip = inet_ntoa(message.sender_addr.sin_addr);
                        wait_for_response = false;
                        new_server_type = ServerType::Replica;
                        new_server_ip = ip;
                        continue;
                    }

                    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_time).count();

                    if (elapsed_time > REQUEST_TIMEOUT_MS)
                    {
                        timed_out = true;
                        wait_for_response = false;
                    }
                }
            }

            logger.debug("Server type: " + std::to_string((int)new_server_type));

            server_type = new_server_type;
            primary_server_ip = new_server_ip;

            if (server_type == ServerType::Replica)
            {
                socket_instance.set_server_ip(primary_server_ip);
                keep_alive();
            }
        }

        void keep_alive()
        {
            std::thread keep_alive_thread([this]()
                                          {
                bool server_alive = true;
                socket_instance.set_timeout(REQUEST_TIMEOUT_MS);

                while (server_alive)
                {
                    logger.debug("Sending keep alive message");
                    socket_instance.send_to_server(KEEP_ALIVE_MESSAGE);

                    bool timed_out = false;
                    bool wait_for_response = true;
                    auto start_time = std::chrono::system_clock::now();

                    while (wait_for_response && !timed_out)
                    {
                        auto message = socket_instance.receive();

                        if (message.is_valid && message.data == IM_ALIVE_MESSAGE)
                        {
                            wait_for_response = false;
                            continue;
                        }

                        if (!message.is_valid)
                        {
                            logger.debug("TIMEOUT 1" + message.data);
                            wait_for_response = false;
                            timed_out = true;
                            server_alive = false;
                            continue;
                        }

                        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_time).count();

                        if (elapsed_time > REQUEST_TIMEOUT_MS)
                        {
                            logger.debug("TIMEOUT 2");
                            wait_for_response = false;
                            timed_out = true;
                            server_alive = false;
                            continue;
                        }
                    }

                    // Espera 500ms
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }

                logger.error("SERVER IS DEAD"); });

            keep_alive_thread.detach();
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

        void process_server_discovery(std::string client_ip)
        {
            logger.debug("Processing server discovery");
            server_map.insert(std::pair<std::string, int>(client_ip, true));
            respond(client_ip);
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