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

    enum class SocketType
    {
        Server,
        Client
    };

    struct ReceivedMessage
    {
        bool is_valid;
        std::string data;
        struct sockaddr_in sender_addr;
    };

    class SocketInstance
    {
    public:
        SocketInstance(int port);
        ~SocketInstance();
        SocketInitResult init();

        int send(const char *message, const struct sockaddr_in &sender_addr);
        int send_broadcast(std::string message);

        ReceivedMessage receive();
        void receiveCallback(const std::function<void(const std::string &data, const struct sockaddr_in &sender_addr)> &callback);
        void stopReceiving();

        void closeConnection();

    private:
        int sock;
        int port;
        bool receiving = false;
        struct sockaddr_in addr, sender_addr;
        socklen_t sender_addr_len = sizeof(sender_addr);

        std::string getLocalIPAddress();
        bool createSocket();
        bool enableBroadcast();
        bool bindInterface();
    };

    SocketInstance::SocketInstance(int port)
    {
        sock = -1;
        this->port = port;
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
        addr.sin_port = htons(this->port);

        return bind(sock, (struct sockaddr *)&addr, sizeof(addr)) >= 0;
    }

    ReceivedMessage SocketInstance::receive()
    {
        ReceivedMessage message;

        char buffer[BUFFER_SIZE];
        int recv_len = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&sender_addr, &sender_addr_len);
        if (recv_len < 0)
        {
            message.is_valid = false;
            message.data = "";
            return message;
        }

        buffer[recv_len] = '\0';
        message.is_valid = true;
        message.data = std::string(buffer);
        message.sender_addr = sender_addr;

        return message;
    }

    int SocketInstance::send(const char *message, const struct sockaddr_in &sender_addr)
    {
        return sendto(sock, message, strlen(message), 0, (struct sockaddr *)&sender_addr, sizeof(sender_addr));
    }

    int SocketInstance::send_broadcast(std::string message)
    {
        sockaddr_in addr;

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        addr.sin_port = htons(this->port);

        return sendto(sock, message.c_str(), message.length(), 0, (struct sockaddr *)&addr, sizeof(addr));
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
