#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h> // Required for sockaddr_in
#include <unistd.h>     // For close()
#include <ifaddrs.h>

#define BROADCAST_PORT 3000
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

        int send(const char *message);
        int receive(char *buffer, int bufferSize);

        void close();

    private:
        int sock;
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
        close();
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
        return (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0;
    }

    bool SocketInstance::enableBroadcast()
    {
        int broadcastEnable = 1;
        return setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0;
    }

    bool SocketInstance::bindInterface()
    {
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(BROADCAST_PORT);

        return bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0;
    }
}

#endif // SOCKET_HPP