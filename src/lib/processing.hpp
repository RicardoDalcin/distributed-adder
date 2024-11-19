#ifndef PROCESSING_HPP
#define PROCESSING_HPP

#include <string>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <thread>

#include "../socket/socket.hpp"
#include "../lib/utils.hpp"
#include "../lib/client_map.hpp"
#include "../logger/logger.hpp"

namespace Processing
{
    struct SharedState
    {
        int num_reqs;
        int total_sum;
    } typedef shared_state_t;

    const std::string REQUEST_MESSAGE = "REQUEST";
    const std::string REQUEST_ACK = "REQUEST_ACK";
    const std::string EXIT_MESSAGE = "EXIT";

    class ProcessingServiceServer
    {
    private:
        SocketInstance::SocketInstance &socket_instance;
        ClientMap::ClientMap &client_map;
        Logger::Logger logger;
        shared_state_t shared_state;
        pthread_mutex_t lock;

    public:
        ProcessingServiceServer(SocketInstance::SocketInstance &socket_instance, ClientMap::ClientMap &client_map)
            : socket_instance(socket_instance), client_map(client_map)
        {
            shared_state.num_reqs = 0;
            shared_state.total_sum = 0;

            int ret = pthread_mutex_init(&lock, NULL);
            if (ret != 0)
            {
                logger.log("Mutex initialization failed with code " + std::to_string(ret));
                exit(1);
            }
        }

        ~ProcessingServiceServer()
        {
            pthread_mutex_destroy(&lock);
        }

        bool is_request_message(std::string message)
        {
            return Utils::starts_with(message, REQUEST_MESSAGE + Utils::DELIMITER);
        }

        void process_request(const struct sockaddr_in &sender_addr, int request_id, int number)
        {
            int partial_sum = 0;

            pthread_mutex_lock(&lock);
            auto client = client_map.get_client(inet_ntoa(sender_addr.sin_addr));

            if (client == nullptr)
            {
                pthread_mutex_unlock(&lock);
                return;
            }

            if (request_id != client->last_req + 1)
            {
                pthread_mutex_unlock(&lock);
                logger.error("Invalid request id: " + std::to_string(request_id) + " last_req: " + std::to_string(client->last_req));
                return;
            }

            // std::this_thread::sleep_for(std::chrono::milliseconds(20));
            shared_state.total_sum += number;
            shared_state.num_reqs++;
            partial_sum = shared_state.total_sum;
            client_map.new_client_request(inet_ntoa(sender_addr.sin_addr), partial_sum);
            pthread_mutex_unlock(&lock);

            respond(sender_addr, request_id, partial_sum);

            logger.log("Sum: " + std::to_string(shared_state.total_sum) + " Num reqs: " + std::to_string(shared_state.num_reqs));
        }

        void respond(const struct sockaddr_in &sender_addr, int request_id, int partial_sum)
        {
            std::string message = REQUEST_ACK + Utils::DELIMITER + std::to_string(request_id) + Utils::DELIMITER + std::to_string(partial_sum);
            socket_instance.send_to(message, sender_addr);
        }

        bool is_exit_message(std::string message)
        {
            return message == EXIT_MESSAGE;
        }

        void handle_exit_message(const struct sockaddr_in &sender_addr)
        {
            logger.log("Client disconnected");
            client_map.remove_client(inet_ntoa(sender_addr.sin_addr));
        }
    };

    class ProcessingServiceClient
    {
    private:
        const int REQUEST_TIMEOUT_MS = 10;

        SocketInstance::SocketInstance &socket_instance;
        Logger::Logger logger;
        int request_id;

    public:
        ProcessingServiceClient(SocketInstance::SocketInstance &socket_instance)
            : socket_instance(socket_instance)
        {
            request_id = 1;

            if (socket_instance.set_timeout(REQUEST_TIMEOUT_MS) != 0)
            {
                logger.error("Failed to set timeout");
                exit(1);
            }
        }

        ~ProcessingServiceClient()
        {
        }

        void send_request(int number)
        {
            std::string message = REQUEST_MESSAGE + Utils::DELIMITER + std::to_string(request_id) + Utils::DELIMITER + std::to_string(number);
            socket_instance.send_to_server(message);

            bool wait_for_response = true;
            bool timed_out = false;

            auto start_time = std::chrono::system_clock::now();

            while (wait_for_response && !timed_out)
            {
                auto message = socket_instance.receive();

                logger.log("Received message: " + message.data);

                if (message.is_valid && Utils::starts_with(message.data, REQUEST_ACK))
                {
                    auto params = Utils::parse_message(message.data);

                    if (params.fields_count < 3)
                    {
                        continue;
                    }

                    int request_id = std::stoi(params.fields[1]);
                    // int partial_sum = std::stoi(params.fields[2]);

                    if (request_id != this->request_id)
                    {
                        continue;
                    }

                    wait_for_response = false;
                    continue;
                }

                if (message.is_valid)
                {
                    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_time).count();

                    if (elapsed_time > REQUEST_TIMEOUT_MS)
                    {
                        timed_out = true;
                        continue;
                    }
                }

                if (!message.is_valid)
                {
                    timed_out = true;
                    continue;
                }
            }

            if (timed_out)
            {
                send_request(number);
                return;
            }

            request_id++;
        }

        void disconnect()
        {
            socket_instance.send_to_server(EXIT_MESSAGE);
        }
    };
}

#endif // PROCESSING_HPP