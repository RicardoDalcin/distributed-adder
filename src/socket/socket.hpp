#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <functional>
#include <sys/time.h>

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
        int set_timeout(int timeout_ms);

        int send_to_ip(const std::string message, std::string ip);
        int send_to_server(const std::string message);
        int send_broadcast(const std::string message);

        void set_server_ip(const std::string server_ip);

        ReceivedMessage receive();

        void close_socket();

    private:
        int sock;
        int port;
        struct sockaddr_in receiver_addr, sender_addr, server_addr;
        socklen_t sender_addr_len = sizeof(sender_addr);

        std::string getLocalIPAddress();
        bool create_socket();
        bool enable_broadcast();
        bool bind_interface();

        int send_to(const std::string message, const struct sockaddr_in &target_addr);
    };

    SocketInstance::SocketInstance(int port)
    {
        sock = -1;
        this->port = port;
    }

    SocketInstance::~SocketInstance()
    {
        close_socket();
    }

    SocketInitResult SocketInstance::init()
    {
        if (!create_socket())
        {
            return SocketInitResult::CreateError;
        }

        if (!enable_broadcast())
        {
            return SocketInitResult::SetOptionsError;
        }

        if (!bind_interface())
        {
            return SocketInitResult::BindError;
        }

        return SocketInitResult::Success;
    }

    int SocketInstance::set_timeout(int timeout_ms)
    {
        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    bool SocketInstance::create_socket()
    {
        return (sock = socket(AF_INET, SOCK_DGRAM, 0)) >= 0;
    }

    bool SocketInstance::enable_broadcast()
    {
        int broadcastEnable = 1;
        return setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) >= 0;
    }

    bool SocketInstance::bind_interface()
    {
        receiver_addr.sin_family = AF_INET;
        receiver_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        receiver_addr.sin_port = htons(this->port);

        return bind(sock, (struct sockaddr *)&receiver_addr, sizeof(receiver_addr)) >= 0;
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

    int SocketInstance::send_to(const std::string message, const struct sockaddr_in &sender_addr)
    {
        return sendto(sock, message.c_str(), strlen(message.c_str()), 0, (struct sockaddr *)&sender_addr, sizeof(sender_addr));
    }

    int SocketInstance::send_to_ip(const std::string message, std::string ip)
    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        addr.sin_port = htons(this->port);

        return send_to(message, addr);
    }

    int SocketInstance::send_to_server(const std::string message)
    {
        return send_to(message, server_addr);
    }

    int SocketInstance::send_broadcast(const std::string message)
    {
        sockaddr_in addr;

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        addr.sin_port = htons(this->port);

        return send_to(message, addr);
    }

    void SocketInstance::set_server_ip(const std::string server_ip)
    {
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
        server_addr.sin_port = htons(this->port);
    }

    void SocketInstance::close_socket()
    {
        close(sock);
        sock = -1;
    }

}

#endif // SOCKET_HPP
