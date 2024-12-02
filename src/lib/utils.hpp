#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <array>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <chrono>

std::string address_cache = "";

namespace Utils
{
    static const char DELIMITER = ';';
    static const char DATA_DELIMITER = ',';

    struct ParsedMessage
    {
        int fields_count;
        std::array<std::string, 4> fields;
    } typedef parsed_message;

    bool starts_with(std::string str, std::string prefix)
    {
        return str.compare(0, prefix.length(), prefix) == 0;
    }

    parsed_message parse_message(std::string message)
    {
        parsed_message parsed_message;
        parsed_message.fields_count = 0;

        size_t pos = 0;
        std::string token;

        while ((pos = message.find(DELIMITER)) != std::string::npos)
        {
            token = message.substr(0, pos);
            message.erase(0, pos + 1);
            parsed_message.fields[parsed_message.fields_count] = token;
            parsed_message.fields_count++;
        }

        parsed_message.fields[parsed_message.fields_count] = message;
        parsed_message.fields_count++;

        return parsed_message;
    }

}

#endif // UTILS_HPP