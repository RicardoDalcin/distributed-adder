#ifndef PROCESSING_HPP
#define PROCESSING_HPP

#include <string>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <pthread.h>

#include "../socket/socket.hpp"
#include "../lib/utils.hpp"
#include "../logger/logger.hpp"

namespace Processing
{
    const std::string REQUEST_MESSAGE = "REQUEST";
    const std::string REQUEST_ACK = "REQUEST_ACK";

    class ProcessingService
    {
    public:
        ProcessingService();
        ~ProcessingService();

        bool is_request_message(std::string message);
        void respond(SocketInstance::SocketInstance &socket_instance, const struct sockaddr_in &sender_addr, int request_id, int partial_sum);
        void process_request(SocketInstance::SocketInstance &socket_instance, const struct sockaddr_in &sender_addr, int request_id, int number);
        void send_request(SocketInstance::SocketInstance &socket_instance, int number);

    private:
        int request_id;
        int shared_sum;

        Logger::Logger logger;

        pthread_mutex_t lock;
    };

    ProcessingService::ProcessingService()
    {
        request_id = 1;
        shared_sum = 0;

        int ret = pthread_mutex_init(&lock, NULL);
        if (ret != 0)
        {
            logger.log("Mutex initialization failed with code " + std::to_string(ret));
            exit(1);
        }
    }

    ProcessingService::~ProcessingService()
    {
    }

    bool ProcessingService::is_request_message(std::string message)
    {
        return Utils::starts_with(message, REQUEST_MESSAGE + Utils::DELIMITER);
    }

    void ProcessingService::process_request(SocketInstance::SocketInstance &socket_instance, const struct sockaddr_in &sender_addr, int request_id, int number)
    {
        pthread_mutex_lock(&lock);
        shared_sum += number;
        pthread_mutex_unlock(&lock);

        respond(socket_instance, sender_addr, request_id, shared_sum);
        logger.log("Sum: " + std::to_string(shared_sum));
    }

    void ProcessingService::respond(SocketInstance::SocketInstance &socket_instance, const struct sockaddr_in &sender_addr, int request_id, int partial_sum)
    {
        std::string message = REQUEST_ACK + Utils::DELIMITER + std::to_string(request_id) + Utils::DELIMITER + std::to_string(partial_sum);
        socket_instance.send_to(message, sender_addr);
    }

    void ProcessingService::send_request(SocketInstance::SocketInstance &socket_instance, int number)
    {
        std::string message = REQUEST_MESSAGE + Utils::DELIMITER + std::to_string(request_id) + Utils::DELIMITER + std::to_string(number);
        socket_instance.send_to_server(message);

        bool wait_for_response = true;

        while (wait_for_response)
        {
            auto message = socket_instance.receive();

            if (message.is_valid && Utils::starts_with(message.data, REQUEST_ACK))
            {
                wait_for_response = false;
            }
        }

        request_id++;
    }
}

#endif // PROCESSING_HPP