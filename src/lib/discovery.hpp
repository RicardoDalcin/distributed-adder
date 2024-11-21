#ifndef DISCOVERY_HPP
#define DISCOVERY_HPP

#include <string>
#include <arpa/inet.h>
#include <ifaddrs.h>

#include "../socket/socket.hpp"
#include "../lib/client_map.hpp"

namespace Discovery
{
    const std::string DISCOVERY_MESSAGE = "DISCOVERY";
    const std::string DISCOVERY_RESPONSE = "DISCOVERY_RESPONSE";

    class DiscoveryServiceServer
    {
    private:
        SocketInstance::SocketInstance &socket_instance;
        ClientMap::ClientMap &client_map;

    public:
        DiscoveryServiceServer(SocketInstance::SocketInstance &socket_instance, ClientMap::ClientMap &client_map)
            : socket_instance(socket_instance), client_map(client_map)
        {
        }

        ~DiscoveryServiceServer() {}

        bool is_discovery_message(std::string message)
        {
            return message == DISCOVERY_MESSAGE;
        }

        void respond(std::string client_ip)
        {
            socket_instance.send_to_ip(DISCOVERY_RESPONSE, client_ip);
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
            // Envia a mensagem de descoberta em broadcast
            socket_instance.send_broadcast(DISCOVERY_MESSAGE);

            std::string server_ip = "";
            bool wait_for_response = true;

            while (wait_for_response)
            {
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