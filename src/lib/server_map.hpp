#ifndef SERVER_MAP_HPP
#define SERVER_MAP_HPP

#include <map>
#include <string>
#include "../logger/logger.hpp"

namespace ServerMap
{
  class ServerMap
  {
  private:
    Logger::Logger logger;

    int server_id = 0;
    int aux_server_id = 1;
    std::map<std::string, int> server_map;

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

    void set_id(int id)
    {
      server_id = id;
    }

    int add_server(std::string server_ip)
    {
      int new_server_id = aux_server_id++;
      server_map.insert(std::pair<std::string, int>(server_ip, new_server_id));
      return new_server_id;
    }

    std::string to_string()
    {
      std::string str = "";

      for (auto it = server_map.begin(); it != server_map.end(); it++)
      {
        str += it->first + Utils::DATA_DELIMITER + std::to_string(it->second) + Utils::DATA_DELIMITER;
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

        server_map.insert(std::pair<std::string, int>(address, server_id));
      }
    }

    std::map<std::string, int> get_map()
    {
      return server_map;
    }
  };
}

#endif // SERVER_MAP_HPP