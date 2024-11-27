#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include "../socket/socket.hpp"

namespace Logger
{
    class Logger
    {
    private:
        std::string get_current_date_time()
        {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            std::tm *time = std::localtime(&in_time_t);
            char buffer[100];
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time);
            return std::string(buffer);
        }

        std::string get_request_log(int request_id, int value, int partial_sum, int num_requests, bool is_duplicate = false)
        {
            return (std::string)(is_duplicate ? " DUP!!" : "") + " id_req " + std::to_string(request_id) + " value " + std::to_string(value) + " num_reqs " + std::to_string(num_requests) + " total_sum " + std::to_string(partial_sum);
        }

    public:
        Logger()
        {
        }

        ~Logger()
        {
        }

        void log(const std::string &message)
        {
            std::cout << message << std::endl;
        }

        void server_request(std::string client_ip, int request_id, int value, int partial_sum, int num_requests, bool is_duplicate = false)
        {
            this->log(
                this->get_current_date_time() + " client " + client_ip + this->get_request_log(request_id, value, partial_sum, num_requests, is_duplicate));
        }

        void client_response(std::string server_ip, int request_id, int value, int partial_sum, int num_requests)
        {
            this->log(
                this->get_current_date_time() + " server " + server_ip + this->get_request_log(request_id, value, partial_sum, num_requests));
        }

#ifdef DEBUG
        void error(const std::string &message)
        {
            std::cerr << "\033[1;31m"
                      << "[ERROR] "
                      << message
                      << "\033[0m"
                      << std::endl;
        }

        void debug(const std::string &message)
        {
            std::cout << "\033[1;34m"
                      << "[DEBUG] "
                      << message
                      << "\033[0m"
                      << std::endl;
        }
#else
        void error([[maybe_unused]] const std::string &message)
        {
        }

        void debug([[maybe_unused]] const std::string &message)
        {
        }
#endif

        void socket_init_error(SocketInstance::SocketInitResult init_result)
        {
            if (init_result == SocketInstance::SocketInitResult::CreateError)
            {
                this->error("An error occurred while creating the socket");
                return;
            }

            if (init_result == SocketInstance::SocketInitResult::SetOptionsError)
            {
                this->error("An error occurred while setting socket options");
                return;
            }

            if (init_result == SocketInstance::SocketInitResult::BindError)
            {
                this->error("An error occurred while binding the socket");
                return;
            }

            this->error("An unexpected initialization error happened with code: " + std::to_string((int)init_result));
        }

        void server_hello()
        {
            this->log(this->get_current_date_time() + " num_reqs 0 total_sum 0");
        }

        void client_hello(std::string server_ip)
        {
            this->log(this->get_current_date_time() + " server_addr " + server_ip);
        }
    };

}

#endif // LOGGER_HPP
