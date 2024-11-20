#ifndef CLIENT_MAP_HPP
#define CLIENT_MAP_HPP

#include <map>
#include <string>
#include <pthread.h>

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
  };
}

#endif // CLIENT_MAP_HPP