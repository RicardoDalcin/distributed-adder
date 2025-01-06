#include <iostream>
#include <map>
#include <thread>
#include <pthread.h>
#include <mutex>
#include <condition_variable>

#include "../socket/socket.hpp"
#include "../lib/discovery.hpp"
#include "../lib/processing.hpp"
#include "../lib/client_map.hpp"
#include "../lib/server_map.hpp"
#include "../lib/election.hpp"
#include "../logger/logger.hpp"

struct ClientEntry
{
    std::string address;
    int last_req;
    int last_sum;
} typedef client_entry;

class Semaphore
{
public:
    explicit Semaphore(int count = 0) : count(count) {}

    // Waits for the semaphore to be signaled
    void wait()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]()
                { return count > 0; });
        count = 0;
    }

    // Signals the semaphore, allowing one waiting thread to proceed
    void signal()
    {
        std::unique_lock<std::mutex> lock(mtx);
        count = 1;
        cv.notify_one();
    }

private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;
};

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

    ClientMap::ClientMap client_map;
    ServerMap::ServerMap server_map;

    Election::Election election(socket_instance, client_map, server_map);

    Discovery::DiscoveryServiceServer discovery_service(socket_instance, client_map, server_map, election);
    Processing::ProcessingServiceServer processing_service(socket_instance, client_map, server_map);

    discovery_service.find_primary_server();

    logger.server_hello();

    while (true)
    {
        auto last_keep_alive_msg = std::chrono::system_clock::now();
        auto last_im_alive_msg = std::chrono::system_clock::now();
        bool is_server_alive = true;
        bool alive_failed = false;
        Semaphore received_im_alive(0);

        bool primary_server_changed = false;
        int keep_alive_tries = 0;

        auto keep_alive = [&alive_failed, &keep_alive_tries, &logger, &socket_instance, &discovery_service, &last_keep_alive_msg, &received_im_alive, &is_server_alive, &election, &primary_server_changed]()
        {
            socket_instance.set_timeout(100);
            while (!discovery_service.is_primary_server() && is_server_alive)
            {
                last_keep_alive_msg = std::chrono::system_clock::now();
                keep_alive_tries++;
                socket_instance.send_to_server(Discovery::KEEP_ALIVE_MESSAGE);

                received_im_alive.wait();

                if (is_server_alive && !alive_failed)
                {
                    keep_alive_tries = 0;
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                }
            }

            if (!discovery_service.is_primary_server())
            {
                logger.debug("Server is dead");
                logger.log("KEEP ALIVE THREAD: Server is dead");

                auto on_server_elected = [&logger, &discovery_service, &primary_server_changed](std::string ip)
                {
                    logger.debug("KEEP ALIVE THREAD: Server " + ip + " elected");
                    discovery_service.set_primary_server(ip);
                    primary_server_changed = true;
                };

                auto on_self_elected = [&logger, &discovery_service, &primary_server_changed]()
                {
                    logger.debug("KEEP ALIVE THREAD: I'm the coordinator");
                    discovery_service.set_self_primary_server();
                    primary_server_changed = true;
                };

                election.start_election(on_server_elected, on_self_elected);
            }
        };

        std::thread(keep_alive).detach();

        while (!primary_server_changed)
        {
            if (election.is_active())
            {
                continue;
            }

            auto message = socket_instance.receive();

            std::string data = "";
            std::string client_ip = "";
            bool is_message_valid = message.is_valid;

            if (message.is_valid)
            {
                data = message.data;
                client_ip = inet_ntoa(message.sender_addr.sin_addr);
            }

            // Callback para processar mensagens recebidas em uma thread separada
            auto process_message = [&alive_failed, &keep_alive_tries, &last_keep_alive_msg, &is_server_alive, &last_im_alive_msg, &is_message_valid, &received_im_alive, &logger, &discovery_service, &processing_service, &client_map, data, client_ip, &election, &primary_server_changed]()
            {
                if (discovery_service.is_primary_server())
                {
                    if (!is_message_valid)
                    {
                        return;
                    }

                    if (processing_service.is_request_message(data))
                    {
                        auto params = Utils::parse_message(data);

                        if (params.fields_count < 3)
                        {
                            logger.error("Invalid request message: " + data);
                            return;
                        }

                        int request_id = std::stoi(params.fields[1]);
                        int number = std::stoi(params.fields[2]);

                        processing_service.process_request(client_ip, request_id, number);
                        return;
                    }

                    if (discovery_service.is_keep_alive_message(data))
                    {
                        discovery_service.im_alive(client_ip);
                        return;
                    }

                    if (discovery_service.is_server_discovery_message(data))
                    {
                        discovery_service.process_server_discovery(client_ip);
                        return;
                    }

                    if (discovery_service.is_client_discovery_message(data))
                    {
                        discovery_service.respond(client_ip);
                        client_map.add_client(client_ip);
                        return;
                    }

                    if (processing_service.is_exit_message(data))
                    {
                        processing_service.handle_exit_message(client_ip);
                        return;
                    }
                }
                else
                {
                    if (!is_message_valid)
                    {
                        // Se a última mensagem de IM_ALIVE foi recebida antes da última mensagem de KEEP_ALIVE,
                        // significa que o servidor está morto
                        auto now = std::chrono::system_clock::now();
                        if (last_im_alive_msg < last_keep_alive_msg && now - last_keep_alive_msg > std::chrono::milliseconds(2000))
                        {
                            if (keep_alive_tries >= 3)
                            {
                                is_server_alive = false;
                            }
                            else
                            {
                                alive_failed = true;
                            }

                            received_im_alive.signal();
                        }

                        return;
                    }

                    if (discovery_service.is_im_alive_message(data))
                    {
                        last_im_alive_msg = std::chrono::system_clock::now();
                        // logger.debug("Im alive message received in processing thread");
                        received_im_alive.signal();
                    }

                    if (processing_service.is_state_update_message(data))
                    {
                        received_im_alive.signal();
                        processing_service.handle_state_update(data);
                        return;
                    }

                    if (discovery_service.is_update_server_list_message(data))
                    {
                        discovery_service.handle_update_server_list_message(data);
                        return;
                    }

                    if (election.is_election_message(data))
                    {
                        auto on_server_elected = [&logger, &discovery_service, &primary_server_changed](std::string ip)
                        {
                            logger.debug("MAIN THREAD: Server " + ip + " elected");
                            discovery_service.set_primary_server(ip);
                            primary_server_changed = true;
                        };

                        auto on_self_elected = [&logger, &discovery_service, &primary_server_changed]()
                        {
                            logger.debug("MAIN THREAD: I'm the coordinator");
                            discovery_service.set_self_primary_server();
                            primary_server_changed = true;
                        };

                        election.handle_election_message(client_ip, on_server_elected, on_self_elected);
                        return;
                    }

                    if (election.is_election_coordinator_message(data))
                    {
                        election.handle_election_coordinator_message(client_ip);
                        return;
                    }
                }
            };

            // Cria uma thread para processar a mensagem
            std::thread(process_message).detach();
        }

        logger.debug("Primary server changed, will restart loop");
    }

    return 0;
}
