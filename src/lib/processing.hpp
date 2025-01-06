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
#include "../lib/server_map.hpp"
#include "../logger/logger.hpp"

namespace Processing
{
    struct SharedState
    {
        int num_reqs;
        uint64_t total_sum;
    } typedef shared_state_t;

    const std::string REQUEST_MESSAGE = "REQUEST";
    const std::string REQUEST_ACK = "REQUEST_ACK";
    const std::string STATE_UPDATE_MESSAGE = "STATE_UPDATE";
    const std::string STATE_UPDATE_ACK = "STATE_UPDATE_ACK";
    const std::string EXIT_MESSAGE = "EXIT";

    class ProcessingServiceServer
    {
    private:
        SocketInstance::SocketInstance &socket_instance;
        ClientMap::ClientMap &client_map;
        ServerMap::ServerMap &server_map;
        Logger::Logger logger;
        shared_state_t shared_state;
        pthread_mutex_t lock;
        bool is_waiting_state_responses = false;

    public:
        ProcessingServiceServer(SocketInstance::SocketInstance &socket_instance, ClientMap::ClientMap &client_map, ServerMap::ServerMap &server_map)
            : socket_instance(socket_instance), client_map(client_map), server_map(server_map)
        {
            shared_state.num_reqs = 0;
            shared_state.total_sum = 0;

            int ret = pthread_mutex_init(&lock, NULL);
            if (ret != 0)
            {
                logger.error("Mutex initialization failed with code " + std::to_string(ret));
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

        bool is_state_update_message(std::string message)
        {
            return Utils::starts_with(message, STATE_UPDATE_MESSAGE + Utils::DELIMITER);
        }

        void send_state_update(std::string client_ip, int partial_sum, int num_requests)
        {
            std::string str_client_map = client_map.to_string();

            logger.debug("Sending state update to " + client_ip);

            socket_instance.send_to_ip(STATE_UPDATE_MESSAGE + Utils::DELIMITER + str_client_map + Utils::DELIMITER + std::to_string(partial_sum) + Utils::DELIMITER + std::to_string(num_requests), client_ip);
        }

        void process_request(std::string client_ip, int request_id, int number)
        {
            int partial_sum = 0;
            int num_requests = 0;

            pthread_mutex_lock(&lock);
            auto client = client_map.get_client(client_ip);

            if (client == nullptr)
            {
                logger.debug("Client not found: " + client_ip);
                pthread_mutex_unlock(&lock);
                return;
            }

            // Verifica se o ID da requisição é o esperado para o cliente
            if (request_id != client->last_req + 1)
            {
                logger.debug("Invalid request id: " + std::to_string(request_id) + " expected: " + std::to_string(client->last_req + 1));
                int last_request = client->last_req;
                int partial_sum = shared_state.total_sum;
                int num_requests = shared_state.num_reqs;

                pthread_mutex_unlock(&lock);

                // Reenvia o ack para o cliente com o ID da última requisição processada
                respond(client_ip, last_request, num_requests, partial_sum);

                if (request_id <= last_request)
                {
                    // Log de requisição duplicada
                    logger.server_request(client_ip, request_id, number, partial_sum, num_requests, true);
                }

                return;
            }

            logger.debug("Request id ok, will process");

            // Atualiza o estado compartilhado
            shared_state.total_sum += number;
            shared_state.num_reqs++;

            // Salva os valores atualizados nas variáveis
            partial_sum = shared_state.total_sum;
            num_requests = shared_state.num_reqs;

            // Atualiza a última requisição processada do cliente
            client_map.new_client_request(client_ip, partial_sum);

            auto update_iterator = [this, &partial_sum, &num_requests](std::pair<std::string, ServerMap::server_t> data)
            {
                if (data.second.is_alive)
                {
                    send_state_update(data.first, partial_sum, num_requests);
                    socket_instance.wait_for_message(STATE_UPDATE_ACK, 50);
                }
            };

            is_waiting_state_responses = true;
            server_map.iterate(update_iterator);
            is_waiting_state_responses = false;

            pthread_mutex_unlock(&lock);

            // Responde ao cliente com o ack
            respond(client_ip, request_id, num_requests, partial_sum);
            logger.server_request(client_ip, request_id, number, partial_sum, num_requests);
        }

        void respond(std::string ip, int request_id, int num_requests, int partial_sum)
        {
            // prettier-ignore
            std::string message =
                REQUEST_ACK + Utils::DELIMITER +
                std::to_string(request_id) + Utils::DELIMITER +
                std::to_string(num_requests) + Utils::DELIMITER +
                std::to_string(partial_sum);

            socket_instance.send_to_ip(message, ip);
        }

        void handle_state_update(std::string message)
        {
            std::string str_client_map = Utils::parse_message(message).fields[1];
            client_map = ClientMap::ClientMap::from_string(str_client_map);

            client_map.for_each([this](std::string client_ip, ClientMap::ClientEntry client_entry)
                                { logger.debug("Client " + client_ip + " last_req: " + std::to_string(client_entry.last_req) + " last_sum: " + std::to_string(client_entry.last_sum)); });

            int partial_sum = std::stoi(Utils::parse_message(message).fields[2]);
            int num_requests = std::stoi(Utils::parse_message(message).fields[3]);

            shared_state.total_sum = partial_sum;
            shared_state.num_reqs = num_requests;
            // respond_state_update();
        }

        void respond_state_update()
        {
            logger.debug("Responding state update");
            std::string message = STATE_UPDATE_ACK + Utils::DELIMITER;
            socket_instance.send_to_server(message);
        }

        bool is_exit_message(std::string message)
        {
            return message == EXIT_MESSAGE;
        }

        void handle_exit_message(std::string client_ip)
        {
            client_map.remove_client(client_ip);
        }
    };

    class ProcessingServiceClient
    {
    private:
        const int REQUEST_TIMEOUT_MS = 500;

        SocketInstance::SocketInstance &socket_instance;
        Logger::Logger logger;
        int request_id;
        int tries = 0;

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

        bool send_request(int number)
        {
            // Envia a requisição ao servidor
            std::string message = REQUEST_MESSAGE + Utils::DELIMITER + std::to_string(request_id) + Utils::DELIMITER + std::to_string(number);
            logger.debug("Sending request to server: " + message + " ip: " + socket_instance.get_server_ip());
            socket_instance.send_to_server(message);

            bool wait_for_response = true;
            bool timed_out = false;

            auto start_time = std::chrono::system_clock::now();

            while (wait_for_response && !timed_out)
            {
                // Espera o ack do servidor
                auto message = socket_instance.receive();

                if (message.is_valid && Utils::starts_with(message.data, REQUEST_ACK))
                {
                    auto params = Utils::parse_message(message.data);

                    if (params.fields_count < 4)
                    {
                        logger.error("Invalid message format: " + message.data);
                        continue;
                    }

                    int request_id = std::stoi(params.fields[1]);
                    int num_requests = std::stoi(params.fields[2]);
                    int partial_sum = std::stoi(params.fields[3]);

                    if (request_id != this->request_id)
                    {
                        continue;
                    }

                    logger.client_response(inet_ntoa(message.sender_addr.sin_addr), request_id, number, partial_sum, num_requests);
                    wait_for_response = false;
                    continue;
                }

                // Verifica se recebeu uma mensagem válida que não seja o ack esperado
                if (message.is_valid)
                {
                    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_time).count();

                    // Se o tempo corrido foi maior que o limite, considera timeout
                    if (elapsed_time > REQUEST_TIMEOUT_MS)
                    {
                        timed_out = true;
                        continue;
                    }
                }

                // Se a mensagem não foi válida, considera timeout
                if (!message.is_valid)
                {
                    timed_out = true;
                    continue;
                }
            }

            if (timed_out)
            {
                logger.debug("Timed out, will retry" + std::to_string(tries));
                if (tries < 3)
                {
                    tries++;
                    bool ok = send_request(number);

                    if (ok)
                    {
                        tries = 0;
                        return true;
                    }

                    return false;
                }
                else
                {
                    return false;
                }
            }

            tries = 0;
            request_id++;
            return true;
        }

        void disconnect()
        {
            socket_instance.send_to_server(EXIT_MESSAGE);
        }
    };
}

#endif // PROCESSING_HPP