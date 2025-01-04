#ifndef CLIENT_MAP_HPP
#define CLIENT_MAP_HPP

#include <map>
#include <string>
#include <pthread.h>
#include <vector>

#include "../logger/logger.hpp"

namespace ClientMap
{
  struct ClientEntry
  {
    std::string address;
    int last_req;
    int last_sum;
  } typedef client_entry;

  class ClientMap
  {
  private:
    Logger::Logger logger;
    pthread_mutex_t lock;
    std::map<std::string, client_entry> client_map;

  public:
    ClientMap()
    {
      int ret = pthread_mutex_init(&lock, NULL);

      if (ret != 0)
      {
        logger.error("Mutex initialization failed with code " + std::to_string(ret));
        exit(1);
      }
    }

    ~ClientMap()
    {
      pthread_mutex_destroy(&lock);
    }

    void add_client(std::string client_ip)
    {
      client_entry new_client;
      new_client.address = client_ip;
      new_client.last_req = 0;
      new_client.last_sum = 0;

      pthread_mutex_lock(&lock);
      client_map.insert(std::pair<std::string, client_entry>(client_ip, new_client));
      pthread_mutex_unlock(&lock);
    }

    void add_existing_client(std::string client_ip, int last_req, int last_sum)
    {
      client_entry new_client;
      new_client.address = client_ip;
      new_client.last_req = last_req;
      new_client.last_sum = last_sum;

      pthread_mutex_lock(&lock);
      client_map.insert(std::pair<std::string, client_entry>(client_ip, new_client));
      pthread_mutex_unlock(&lock);
    }

    void remove_client(std::string client_ip)
    {
      pthread_mutex_lock(&lock);
      client_map.erase(client_ip);
      pthread_mutex_unlock(&lock);
    }

    client_entry *get_client(std::string client_ip)
    {
      pthread_mutex_lock(&lock);
      auto it = client_map.find(client_ip);
      if (it != client_map.end())
      {
        pthread_mutex_unlock(&lock);
        return &it->second;
      }
      pthread_mutex_unlock(&lock);
      return nullptr;
    }

    void new_client_request(std::string client_ip, int last_sum)
    {
      client_entry *client = get_client(client_ip);

      if (client == nullptr)
      {
        logger.error("Client not found: " + client_ip);
        return;
      }

      pthread_mutex_lock(&lock);
      client->last_req++;
      client->last_sum = last_sum;
      pthread_mutex_unlock(&lock);
    }

    std::string to_string()
    {
      std::string str = "";

      pthread_mutex_lock(&lock);
      for (auto it = client_map.begin(); it != client_map.end(); it++)
      {
        str += it->first + Utils::DATA_DELIMITER + std::to_string(it->second.last_req) + Utils::DATA_DELIMITER + std::to_string(it->second.last_sum) + Utils::DATA_DELIMITER;
      }
      pthread_mutex_unlock(&lock);

      return str;
    }

    static ClientMap from_string(std::string value)
    {
      ClientMap client_map;

      size_t pos = 0;
      std::string token;

      std::vector<std::string> tokens;

      while ((pos = value.find(Utils::DATA_DELIMITER)) != std::string::npos)
      {
        token = value.substr(0, pos);
        value.erase(0, pos + 1);
        tokens.push_back(token);
      }

      for (size_t i = 0; i < tokens.size(); i += 3)
      {
        std::string address = tokens[i];
        int last_req = std::stoi(tokens[i + 1]);
        int last_sum = std::stoi(tokens[i + 2]);

        client_map.add_existing_client(address, last_req, last_sum);
      }

      return client_map;
    }

    int size()
    {
      pthread_mutex_lock(&lock);
      int size = client_map.size();
      pthread_mutex_unlock(&lock);
      return size;
    }

    void for_each(std::function<void(std::string, client_entry)> callback)
    {
      pthread_mutex_lock(&lock);
      for (auto it = client_map.begin(); it != client_map.end(); it++)
      {
        callback(it->first, it->second);
      }
      pthread_mutex_unlock(&lock);
    }
  };
}

#endif // CLIENT_MAP_HPP