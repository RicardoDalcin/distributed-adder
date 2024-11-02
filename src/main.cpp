#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h> // Required for sockaddr_in
#include <unistd.h>     // For close()
#include <ifaddrs.h>
#include <pthread.h>

#include "socket/socket.hpp"

#define NUM_THREADS 3

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

struct thread_info
{
    pthread_t id;
    int num;
};

int counter = 0;
pthread_mutex_t lock;
SocketInstance::SocketInstance socket_inst = SocketInstance::SocketInstance();

static void *thread_start(void *arg)
{
    struct thread_info *thread_info = (struct thread_info *)arg;
    std::cout << "Thread " << thread_info->num << " started" << std::endl;

    auto onReceive = [](const std::string &data, const struct sockaddr_in &sender_addr)
    {
        std::cout << "Received message: " << data << " from " << inet_ntoa(sender_addr.sin_addr) << "\n";
        pthread_mutex_lock(&lock);
        counter += std::stoi(data);
        pthread_mutex_unlock(&lock);

        std::cout << "Counter: " << counter << "\n";

        std::string localIP = getLocalIPAddress();
        int sent_len = const_cast<SocketInstance::SocketInstance &>(socket_inst).send(localIP.c_str(), sender_addr);
        if (sent_len < 0)
        {
            std::cout << "Send failed" << std::endl;
        }

        sleep(5);
        std::cout << "Stopped sleeping" << std::endl;
    };

    socket_inst.receiveCallback(onReceive);
    socket_inst.closeConnection();

    return 0x0;
}

int main()
{
    pthread_attr_t attr;
    void *res;

    struct thread_info thread_info[NUM_THREADS];

    SocketInstance::SocketInitResult initResult = socket_inst.init();

    if (initResult != SocketInstance::SocketInitResult::Success)
    {
        std::cerr << "Socket initialization failed with code " << (int)initResult << std::endl;
        return 0x0;
    }

    std::cout << "Listening to messages on port " << BROADCAST_PORT << "...\n";

    int ret = pthread_mutex_init(&lock, NULL);
    if (ret != 0)
    {
        std::cerr << "Mutex initialization failed with code " << ret << std::endl;
        return 1;
    }

    ret = pthread_attr_init(&attr);
    if (ret != 0)
    {
        std::cerr << "Attribute initialization failed with code " << ret << std::endl;
        return 1;
    }

    for (int thread_num = 0; thread_num < NUM_THREADS; thread_num++)
    {
        thread_info[thread_num].num = thread_num + 1;

        ret = pthread_create(&thread_info[thread_num].id, &attr, &thread_start, &thread_info[thread_num]);
        if (ret != 0)
        {
            std::cerr << "Thread creation failed with code " << ret << std::endl;
            return 1;
        }
    }

    ret = pthread_attr_destroy(&attr);
    if (ret != 0)
    {
        std::cerr << "Attribute destruction failed with code " << ret << std::endl;
        return 1;
    }

    for (int thread_num = 0; thread_num < NUM_THREADS; thread_num++)
    {
        ret = pthread_join(thread_info[thread_num].id, &res);
        if (ret != 0)
        {
            std::cerr << "Thread join failed with code " << ret << std::endl;
            return 1;
        }

        std::cout << "Thread " << thread_info[thread_num].num << " joined" << std::endl;
        free(res);
    }

    pthread_mutex_destroy(&lock);

    return 0;

    // SocketInstance::SocketInstance socket = SocketInstance::SocketInstance();
    // SocketInstance::SocketInitResult initResult = socket.init();

    // std::string localIP = getLocalIPAddress();

    // if (initResult != SocketInstance::SocketInitResult::Success)
    // {
    //     std::cerr << "Socket initialization failed with code " << (int)initResult << std::endl;
    //     return 1;
    // }

    // std::cout << "Listening to messages on port " << BROADCAST_PORT << "...\n";

    // auto onReceive = [&counter, socket](const std::string &data, const struct sockaddr_in &sender_addr)
    // {
    //     std::cout << "Received message: " << data << " from " << inet_ntoa(sender_addr.sin_addr) << "\n";
    //     counter += std::stoi(data);
    //     std::cout << "Counter: " << counter << "\n";

    //     std::string localIP = getLocalIPAddress();
    //     int sent_len = const_cast<SocketInstance::SocketInstance &>(socket).send(localIP.c_str(), sender_addr);
    //     if (sent_len < 0)
    //     {
    //         std::cout << "Send failed" << std::endl;
    //     }

    //     sleep(5);
    //     std::cout << "Stopped sleeping" << std::endl;
    // };

    // while (true)
    // {
    //     socket.receiveCallback(onReceive);
    // }

    // socket.closeConnection();
    return 0;
}
