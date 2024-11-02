#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h> // Required for sockaddr_in
#include <unistd.h>     // For close()
#include <ifaddrs.h>

#include "socket/socket.hpp"

#define BROADCAST_PORT 3000
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
    SocketInstance::SocketInstance socket = SocketInstance::SocketInstance();
    SocketInstance::SocketInitResult initResult = socket.init();

    std::string localIP = getLocalIPAddress();

    while (true)
    {
        // 4. Receive broadcast message
        int recv_len = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&sender_addr, &sender_addr_len);
        if (recv_len < 0)
        {
            perror("Receive failed");
            break;
        }

        buffer[recv_len] = '\0';
        std::cout << "Received message: " << buffer << "\n";

        // 5. Send response back with our IP address
        std::string localIP = getLocalIPAddress();
        int sent_len = sendto(sock, localIP.c_str(), localIP.length(), 0, (struct sockaddr *)&sender_addr, sender_addr_len);
        if (sent_len < 0)
        {
            perror("Send failed");
            break;
        }

        std::cout << "Sent local IP address (" << localIP << ") to sender\n";
    }

    close(sock);
    return 0;
}
