#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <random>

#define BROADCAST_PORT 12345
#define BUFFER_SIZE 1024
#define BROADCAST_MESSAGE "Hello, any server there?"
#define IGNORE_BROADCAST true
#define DIRECT_IP_ADDRESS "192.168.0.5"

int main()
{
    int sock;
    struct sockaddr_in broadcast_addr, response_addr;
    socklen_t response_addr_len = sizeof(response_addr);
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

    // 3. Configure the broadcast address
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;

    if (IGNORE_BROADCAST)
        broadcast_addr.sin_addr.s_addr = inet_addr(DIRECT_IP_ADDRESS);
    else
        broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST); // Send to broadcast address

    broadcast_addr.sin_port = htons(BROADCAST_PORT);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 100);
    int randomNumber = dis(gen);
    const char *message = std::to_string(randomNumber).c_str();

    // 4. Send the broadcast message
    if (sendto(sock, message, strlen(message), 0,
               (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) < 0)
    {
        perror("Broadcast send failed");
        close(sock);
        return 1;
    }

    std::cout << "Broadcast message sent: " << message << "\n";
    std::cout << "Waiting for server response...\n";

    // 5. Wait for a response
    int recv_len = recvfrom(sock, buffer, BUFFER_SIZE, 0,
                            (struct sockaddr *)&response_addr, &response_addr_len);
    if (recv_len < 0)
    {
        perror("Receive failed");
        close(sock);
        return 1;
    }

    buffer[recv_len] = '\0'; // Null-terminate the received message
    std::cout << "Received response from server: " << buffer << "\n";

    // 6. Print server IP address
    char sender_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &response_addr.sin_addr, sender_ip, sizeof(sender_ip));
    std::cout << "Server IP Address: " << sender_ip << "\n";

    close(sock);
    return 0;
}
