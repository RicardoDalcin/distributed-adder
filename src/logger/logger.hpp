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
    public:
        Logger();
        ~Logger();

        void log(const std::string &message);
        void error(const std::string &message);
        void socket_init_error(SocketInstance::SocketInitResult init_result);
        void server_hello();
        void client_hello(std::string server_ip);

    private:
        std::string get_current_date_time();
    };

    Logger::Logger()
    {
    }

    Logger::~Logger()
    {
    }

    void Logger::log(const std::string &message)
    {
        std::cout << message << std::endl;
    }

    void Logger::error(const std::string &message)
    {
        std::cerr << "\033[1;31m"
                  << "[ERROR] "
                  << message
                  << "\033[0m"
                  << std::endl;
    }

    void Logger::socket_init_error(SocketInstance::SocketInitResult init_result)
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

    void Logger::server_hello()
    {
        this->log(this->get_current_date_time() + " num_reqs 0 total_sum 0");
    }

    void Logger::client_hello(std::string server_ip)
    {
        this->log(this->get_current_date_time() + " server_addr " + server_ip);
    }

    std::string Logger::get_current_date_time()
    {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm *time = std::localtime(&in_time_t);
        char buffer[100];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time);
        return std::string(buffer);
    }
}

#endif // LOGGER_HPP
