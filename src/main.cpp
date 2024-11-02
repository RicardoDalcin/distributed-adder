#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h> // Required for sockaddr_in
#include <unistd.h>     // For close()
#include <ifaddrs.h>

#include "socket/socket.hpp"

#define BUFFER_SIZE 1024

std::string getLocalIPAddress()
{
    struct ifaddrs *interfaces = nullptr;
    getifaddrs(&interfaces);

    std::string localIP;
    for (struct ifaddrs *iface = interfaces; iface != nullptr; iface = iface->ifa_next)
    {
        if (iface->ifa_addr->sa_family == AF_INET)
        { // Check for IPv4
            char address[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &((struct sockaddr_in *)iface->ifa_addr)->sin_addr, address, sizeof(address));
            if (strcmp(iface->ifa_name, "lo") != 0)
            { // Exclude loopback interface
                localIP = address;
                break; // Get the first non-loopback address
            }
        }
    }

    freeifaddrs(interfaces);
    return localIP.empty() ? "127.0.0.1" : localIP; // Fallback if no address found
}

int main()
{
    int counter = 0;

    SocketInstance::SocketInstance socket = SocketInstance::SocketInstance();
    SocketInstance::SocketInitResult initResult = socket.init();

    std::string localIP = getLocalIPAddress();

    if (initResult != SocketInstance::SocketInitResult::Success)
    {
        std::cerr << "Socket initialization failed with code " << (int)initResult << std::endl;
        return 1;
    }

    std::cout << "Listening to messages on port " << BROADCAST_PORT << "...\n";

    auto onReceive = [&counter](const std::string &data)
    {
        std::cout << "Received message: " << data << "\n";
        counter += std::stoi(data);
        std::cout << "Counter: " << counter << "\n";
    };

    while (true)
    {
        socket.receiveCallback(onReceive);
    }

    socket.closeConnection();
    return 0;
}
