#ifndef SERVER_MAP_HPP
#define SERVER_MAP_HPP

#include <map>
#include <string>
#include "../logger/logger.hpp"

namespace ServerMap
{
  struct server
  {
    int id;
    bool is_alive;
  } typedef server_t;

  class ServerMap
  {
  private:
    Logger::Logger logger;

    int server_id = 0;
    int aux_server_id = 1;
    std::map<std::string, server_t> server_map;

  public:
    ServerMap()
    {
    }

    ~ServerMap()
    {
    }

    int get_id()
    {
      return server_id;
    }

    int get_id_from_ip(std::string ip)
    {
      auto it = server_map.find(ip);
      if (it != server_map.end())
      {
        return it->second.id;
      }

      return -1;
    }

    void set_id(int id)
    {
      server_id = id;
    }

    int add_server(std::string server_ip)
    {
      int new_server_id = aux_server_id++;
      server_t new_server;
      new_server.id = new_server_id;
      new_server.is_alive = true;

      server_map.insert(std::pair<std::string, server_t>(server_ip, new_server));
      return new_server_id;
    }

    std::string to_string()
    {
      std::string str = "";

      for (auto it = server_map.begin(); it != server_map.end(); it++)
      {
        str += it->first + Utils::DATA_DELIMITER + std::to_string(it->second.id) + Utils::DATA_DELIMITER;
      }

      return str;
    }

    void load_from_string(std::string value)
    {
      server_map.clear();
      size_t pos = 0;
      std::string token;

      std::vector<std::string> tokens;

      while ((pos = value.find(Utils::DATA_DELIMITER)) != std::string::npos)
      {
        token = value.substr(0, pos);
        value.erase(0, pos + 1);
        tokens.push_back(token);
      }

      for (size_t i = 0; i < tokens.size(); i += 2)
      {
        std::string address = tokens[i];
        int server_id = std::stoi(tokens[i + 1]);
        server_t server;
        server.id = server_id;
        server.is_alive = true;

        server_map.insert(std::pair<std::string, server_t>(address, server));
      }
    }

    void iterate(std::function<void(std::pair<std::string, server_t>)> callback)
    {
      for (auto it = server_map.begin(); it != server_map.end(); it++)
      {
        if (it->second.id != server_id)
        {
          callback(std::pair<std::string, server_t>(it->first, it->second));
        }
      }
    }

    void iterate_higher_priority(std::function<void(std::pair<std::string, server_t>)> callback)
    {
      for (auto it = server_map.begin(); it != server_map.end(); it++)
      {
        if (it->second.id < server_id)
        {
          callback(std::pair<std::string, server_t>(it->first, it->second));
        }
      }
    }

    void disable_higher_priority_servers()
    {
      for (auto it = server_map.begin(); it != server_map.end(); it++)
      {
        if (it->second.id < server_id)
        {
          it->second.is_alive = false;
        }
      }
    }
  };
}

#endif // SERVER_MAP_HPP