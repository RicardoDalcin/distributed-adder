#include <iostream>
#include <signal.h>
#include <string.h>
#include "../socket/socket.hpp"
#include "../logger/logger.hpp"
#include "../lib/discovery.hpp"
#include "../lib/processing.hpp"
#include "../lib/utils.hpp"

struct sigaction old_action;

std::function<void(int)> signal_handler_callback;

void signal_handler(int signo)
{
    if (signal_handler_callback)
    {
        signal_handler_callback(signo);
    }
}

int main(int argc, char *argv[])
{
    Logger::Logger logger;

    if (argc < 2)
    {
        logger.error("Missing required argument: port");
        return 1;
    }

    int port = atoi(argv[1]);

    if (port < 1 || port > 65535)
    {
        logger.error("Invalid port number: " + std::to_string(port));
        return 1;
    }

    auto socket_instance = SocketInstance::SocketInstance(port);
    auto init_result = socket_instance.init();

    if (init_result != SocketInstance::SocketInitResult::Success)
    {
        logger.socket_init_error(init_result);
        return 1;
    }

    Discovery::DiscoveryServiceClient discovery_service(socket_instance);
    Processing::ProcessingServiceClient processing_service(socket_instance);

    bool is_first_server = true;
    bool finished = false;
    int timed_out_number = -1;

    while (!finished)
    {
        logger.debug("Inner loop started");
        std::string server_ip = discovery_service.find_server_ip();
        is_first_server = false;
        bool timed_out = false;

        if (server_ip.empty())
        {
            logger.error("Discovery failed. No server IP address found.");
            return 1;
        }

        if (is_first_server)
        {
            logger.client_hello(server_ip);
        }
        socket_instance.set_server_ip(server_ip);

        signal_handler_callback = [&processing_service](int signo)
        {
            if (signo == SIGINT)
            {
                // Envia mensagem de exit para o servidor caso receba um SIGINT
                processing_service.disconnect();
                sigaction(SIGINT, &old_action, NULL);
                kill(0, SIGINT);
            }
        };

        struct sigaction action;
        memset(&action, 0, sizeof(action));
        action.sa_handler = signal_handler;
        sigaction(SIGINT, &action, &old_action);

        if (timed_out_number != -1)
        {
            logger.debug("Retrying request " + std::to_string(timed_out_number));
            bool ok = processing_service.send_request(timed_out_number);

            if (ok)
            {
                timed_out_number = -1;
            }
            else
            {
                timed_out = true;
            }
        }

        if (timed_out)
        {
            continue;
        }

        std::string line;
        while (!timed_out && std::getline(std::cin, line))
        {
            try
            {
                int input_number = std::stoi(line);
                bool ok = processing_service.send_request(input_number);

                if (!ok)
                {
                    logger.debug("Timed out, will search for server again");
                    timed_out_number = input_number;
                    timed_out = true;
                }
            }
            catch (std::invalid_argument &e)
            {
                logger.error("Invalid number: " + line);
            }
        }

        if (timed_out)
        {
            continue;
        }

        logger.debug("Disconnecting");
        processing_service.disconnect();
        finished = true;
    }

    logger.debug("Exiting");
    return 0;
}
