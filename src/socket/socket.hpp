#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h> // Required for sockaddr_in
#include <unistd.h>     // For close()
#include <ifaddrs.h>
#include <functional>

#define BROADCAST_PORT 12345
#define BUFFER_SIZE 1024

namespace SocketInstance
{
    enum class SocketInitResult
    {
        Success,
        CreateError,
        SetOptionsError,
        BindError,
    };

    class SocketInstance
    {
    public:
        SocketInstance();
        ~SocketInstance();
        SocketInitResult init();

        int send(const char *message, const struct sockaddr_in &sender_addr);
        int receive(char *buffer, int bufferSize);
        void receiveCallback(const std::function<void(const std::string &data, const struct sockaddr_in &sender_addr)> &callback);
        void stopReceiving();

        void closeConnection();

    private:
        int sock;
        bool receiving = false;
        struct sockaddr_in addr, sender_addr;
        socklen_t sender_addr_len = sizeof(sender_addr);

        std::string getLocalIPAddress();
        bool createSocket();
        bool enableBroadcast();
        bool bindInterface();
    };

    SocketInstance::SocketInstance()
    {
        sock = -1;
    }

    SocketInstance::~SocketInstance()
    {
        closeConnection();
    }

    SocketInitResult SocketInstance::init()
    {
        if (!createSocket())
        {
            return SocketInitResult::CreateError;
        }

        if (!enableBroadcast())
        {
            return SocketInitResult::SetOptionsError;
        }

        if (!bindInterface())
        {
            return SocketInitResult::BindError;
        }

        return SocketInitResult::Success;
    }

    bool SocketInstance::createSocket()
    {
        return (sock = socket(AF_INET, SOCK_DGRAM, 0)) >= 0;
    }

    bool SocketInstance::enableBroadcast()
    {
        int broadcastEnable = 1;
        return setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) >= 0;
    }

    bool SocketInstance::bindInterface()
    {
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(BROADCAST_PORT);

        return bind(sock, (struct sockaddr *)&addr, sizeof(addr)) >= 0;
    }

    int SocketInstance::receive(char *buffer, int bufferSize)
    {
        return recvfrom(sock, buffer, bufferSize, 0, (struct sockaddr *)&sender_addr, &sender_addr_len);
    }

    int SocketInstance::send(const char *message, const struct sockaddr_in &sender_addr)
    {
        return sendto(sock, message, strlen(message), 0, (struct sockaddr *)&sender_addr, sizeof(sender_addr));
    }

    void SocketInstance::receiveCallback(const std::function<void(const std::string &data, const struct sockaddr_in &sender_addr)> &callback)
    {
        receiving = true;
        char buffer[BUFFER_SIZE];

        while (receiving)
        {
            int recv_len = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&sender_addr, &sender_addr_len);
            if (recv_len < 0)
            {
                std::cerr << "Receive failed" << std::endl;
                stopReceiving();
            }

            buffer[recv_len] = '\0';
            callback(buffer, sender_addr);
        }
    }

    void SocketInstance::closeConnection()
    {
        close(sock);
        sock = -1;
    }

    void SocketInstance::stopReceiving()
    {
        receiving = false;
    }
}

#endif // SOCKET_HPP
