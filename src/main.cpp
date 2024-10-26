#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h> // Required for sockaddr_in
#include <unistd.h>     // For close()
#include <ifaddrs.h>

#define BROADCAST_PORT 12345
#define BUFFER_SIZE 1024

std::string calculateBroadcastAddress(const std::string &ipAddress, const std::string &subnetMask)
{
    struct in_addr ip, mask, broadcast;

    // Convert IP and mask to binary form
    inet_aton(ipAddress.c_str(), &ip);
    inet_aton(subnetMask.c_str(), &mask);

    // Calculate broadcast address
    broadcast.s_addr = (ip.s_addr & mask.s_addr) | ~mask.s_addr;

    return inet_ntoa(broadcast);
}

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
    int sock;
    struct sockaddr_in addr, sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    char buffer[BUFFER_SIZE];

    // 1. Create a UDP socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        return 1;
    }

    // 2. Enable broadcast on this socket
    int broadcastEnable = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0)
    {
        perror("Error setting broadcast option");
        close(sock);
        return 1;
    }

    std::string localIP = getLocalIPAddress();
    std::string subnetMask = "255.255.255.0"; // Replace with your actual subnet mask
    std::string broadcastIP = calculateBroadcastAddress(localIP, subnetMask);

    std::cout << "Broadcast IP address: " << broadcastIP << "\n"; // Add this line

    // 3. Bind the socket to listen for broadcasts on the specified port
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(BROADCAST_PORT);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Bind failed");
        close(sock);
        return 1;
    }

    std::cout << "Listening for broadcast messages on port " << BROADCAST_PORT << "...\n";

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
