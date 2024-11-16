#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <random>
#include "client_socket.hpp"

#define BUFFER_SIZE 1024
#define NUM_THREADS 3
#define NUM_MESSAGES 1000

pthread_mutex_t lock;
ClientSocket::ClientSocket socket_inst = ClientSocket::ClientSocket();

struct thread_info
{
    pthread_t id;
    int num;
};

static void *thread_start(void *arg)
{
    struct thread_info *thread_info = (struct thread_info *)arg;
    std::cout << "Thread " << thread_info->num << " started" << std::endl;

    char buffer[BUFFER_SIZE];

    for (int i = 0; i < NUM_MESSAGES; i++)
    {
        socket_inst.send("1");
        // socket_inst.receive(buffer, BUFFER_SIZE);
    }

    return 0x0;
}

int main()
{
    pthread_attr_t attr;
    void *res;

    struct thread_info thread_info[NUM_THREADS];

    ClientSocket::SocketInitResult initResult = socket_inst.init();

    if (initResult != ClientSocket::SocketInitResult::Success)
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
}
