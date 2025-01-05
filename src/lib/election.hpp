#ifndef ELECTION_HPP
#define ELECTION_HPP

#include <map>
#include <string>

#include "../socket/socket.hpp"
#include "../lib/client_map.hpp"
#include "../lib/server_map.hpp"
#include "../logger/logger.hpp"

namespace Election
{
  class Election
  {
  private:
    const std::string ELECTION_START_MESSAGE = "ELECTION";
    const std::string ELECTION_ANSWER_MESSAGE = "ELECTION_ANSWER";
    const std::string ELECTION_COORDINATOR_MESSAGE = "ELECTION_COORDINATOR";

    Logger::Logger logger;

    int last_election_result_timestamp = 0;

    SocketInstance::SocketInstance &socket_instance;
    ClientMap::ClientMap &client_map;
    ServerMap::ServerMap &server_map;

    bool is_election_running = false;
    bool has_elected = false;
    std::string elected_server_ip = "";
    std::map<std::string, bool> answer_buffer;

  public:
    Election(SocketInstance::SocketInstance &socket_instance, ClientMap::ClientMap &client_map, ServerMap::ServerMap &server_map)
        : socket_instance(socket_instance), client_map(client_map), server_map(server_map)
    {
    }

    ~Election()
    {
    }

    bool is_active()
    {
      return is_election_running;
    }

    void start_election()
    {
      if (is_election_running)
      {
        logger.debug("Election already running");
        return;
      }

      socket_instance.set_timeout(5);
      is_election_running = true;
      logger.debug("Starting election");
      bool had_any_answer = false;
      auto server_iterator = [this, &had_any_answer](std::pair<std::string, int> data)
      {
        if (has_elected)
        {
          return;
        }

        logger.debug("Sending election message to " + data.first);
        socket_instance.send_to_ip(ELECTION_START_MESSAGE, data.first);
        bool answered = wait_for_answer(data.first);

        if (has_elected)
        {
          return;
        }

        if (answered)
        {
          had_any_answer = true;
        }
      };

      server_map.iterate_higher_priority(server_iterator);

      if (has_elected)
      {
        logger.debug("Election already finished");
        return;
      }

      if (!had_any_answer)
      {
        logger.debug("No answer received, meaning I'm the coordinator!!!");
        last_election_result_timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        server_map.iterate([this](std::pair<std::string, int> data)
                           { send_election_coordinator_message(data.first); });
      }
    }

    bool wait_for_answer(std::string target_ip)
    {
      bool answered = false;
      bool timed_out = false;

      auto start_time = std::chrono::system_clock::now();
      logger.debug("Waiting for answer from " + target_ip);
      while (!answered && !timed_out && !has_elected)
      {
        auto message = socket_instance.receive();

        if (!message.is_valid)
        {
          if (start_time + std::chrono::milliseconds(600) < std::chrono::system_clock::now())
          {
            logger.debug("Timed out");
            timed_out = true;
          }
          continue;
        }

        std::string msg_sender_ip = inet_ntoa(message.sender_addr.sin_addr);

        if (is_election_message(message.data))
        {
          logger.debug("Election message received from " + msg_sender_ip);
          answer_election(msg_sender_ip);
          continue;
        }

        if (is_election_answer_message(message.data))
        {
          if (msg_sender_ip == target_ip)
          {
            answered = true;
            logger.debug("Answer received from " + target_ip);
            continue;
          }

          logger.debug("Answer received from other server, should buffer");
        }

        if (is_election_coordinator_message(message.data))
        {
          has_elected = true;
          elected_server_ip = msg_sender_ip;
          logger.debug("Elected server " + elected_server_ip);
          continue;
        }
      }

      return answered;
    }

    bool is_election_message(std::string message)
    {
      return Utils::starts_with(message, ELECTION_START_MESSAGE + Utils::DELIMITER);
    }

    bool is_election_answer_message(std::string message)
    {
      return Utils::starts_with(message, ELECTION_ANSWER_MESSAGE + Utils::DELIMITER);
    }

    bool is_election_coordinator_message(std::string message)
    {
      return Utils::starts_with(message, ELECTION_COORDINATOR_MESSAGE + Utils::DELIMITER);
    }

    void handle_election_message(std::string client_ip)
    {
      answer_election(client_ip);
      start_election();
    }

    void answer_election(std::string ip)
    {
      socket_instance.send_to_ip(ELECTION_ANSWER_MESSAGE + Utils::DELIMITER, ip);
    }

    void send_election_coordinator_message(std::string ip)
    {
      socket_instance.send_to_ip(ELECTION_COORDINATOR_MESSAGE + Utils::DELIMITER, ip);
    }

    void handle_election_coordinator_message(std::string ip)
    {
      has_elected = true;
      elected_server_ip = ip;
      logger.debug("Elected server from main thread " + elected_server_ip);
    }
  };
}

#endif // ELECTION_HPP