#ifndef CLIENT_SOCKET_HPP
#define CLIENT_SOCKET_HPP

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

namespace ClientSocket
{
  enum class SocketInitResult
  {
    Success,
    CreateError,
    SetOptionsError,
    BindError,
  };

  class ClientSocket
  {
  public:
    ClientSocket();
    ~ClientSocket();
    SocketInitResult init();

    int send(const char *message);
    int receive(char *buffer, int bufferSize);
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

  ClientSocket::ClientSocket()
  {
    sock = -1;
  }

  ClientSocket::~ClientSocket()
  {
    closeConnection();
  }

  SocketInitResult ClientSocket::init()
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

  bool ClientSocket::createSocket()
  {
    return (sock = socket(AF_INET, SOCK_DGRAM, 0)) >= 0;
  }

  bool ClientSocket::enableBroadcast()
  {
    int broadcastEnable = 1;
    return setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) >= 0;
  }

  bool ClientSocket::bindInterface()
  {
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    addr.sin_port = htons(BROADCAST_PORT);

    return true;
  }

  int ClientSocket::receive(char *buffer, int bufferSize)
  {
    return recvfrom(sock, buffer, bufferSize, 0, (struct sockaddr *)&sender_addr, &sender_addr_len);
  }

  int ClientSocket::send(const char *message)
  {
    return sendto(sock, message, strlen(message), 0, (struct sockaddr *)&addr, sizeof(addr));
  }

  void ClientSocket::closeConnection()
  {
    close(sock);
    sock = -1;
  }

  void ClientSocket::stopReceiving()
  {
    receiving = false;
  }
}

#endif // CLIENT_SOCKET_HPP