#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h> // Required for sockaddr_in
#include <unistd.h>     // For close()

#define BROADCAST_PORT 12345
#define BUFFER_SIZE 1024

std::string getLocalIPAddress()
{
    // Replace with code to retrieve actual local IP if needed
    return "127.0.0.1";
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
