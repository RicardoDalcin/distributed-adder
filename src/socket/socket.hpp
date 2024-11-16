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

        int send_to(const std::string message, const struct sockaddr_in &target_addr);
        int send_to_server(const std::string message);
        int send_broadcast(const std::string message);

        void set_server_ip(const std::string server_ip);

        ReceivedMessage receive();
        void receive_callback(const std::function<void(const std::string &data, const struct sockaddr_in &sender_addr)> &callback);
        void stop_receiving();

        void close_socket();

    private:
        int sock;
        int port;
        bool receiving = false;
        struct sockaddr_in receiver_addr, sender_addr, server_addr;
        socklen_t sender_addr_len = sizeof(sender_addr);

        std::string getLocalIPAddress();
        bool create_socket();
        bool enable_broadcast();
        bool bind_interface();
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

    int SocketInstance::send_to_server(const std::string message)
    {
        return sendto(sock, message.c_str(), strlen(message.c_str()), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    int SocketInstance::send_broadcast(const std::string message)
    {
        sockaddr_in addr;

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        addr.sin_port = htons(this->port);

        return sendto(sock, message.c_str(), message.length(), 0, (struct sockaddr *)&addr, sizeof(addr));
    }

    void SocketInstance::set_server_ip(const std::string server_ip)
    {
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
        server_addr.sin_port = htons(this->port);
    }

    void SocketInstance::receive_callback(const std::function<void(const std::string &data, const struct sockaddr_in &sender_addr)> &callback)
    {
        receiving = true;
        char buffer[BUFFER_SIZE];

        while (receiving)
        {
            int recv_len = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&sender_addr, &sender_addr_len);
            if (recv_len < 0)
            {
                std::cerr << "Receive failed" << std::endl;
                stop_receiving();
            }

            buffer[recv_len] = '\0';
            callback(buffer, sender_addr);
        }
    }

    void SocketInstance::close_socket()
    {
        close(sock);
        sock = -1;
    }

    void SocketInstance::stop_receiving()
    {
        receiving = false;
    }
}

#endif // SOCKET_HPP
