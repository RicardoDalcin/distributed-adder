#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <arpa/inet.h>
#include <ifaddrs.h>

std::string address_cache = "";

namespace Utils
{
    std::string get_local_ip_address()
    {
        if (address_cache.empty())
        {
            struct ifaddrs *interfaces = nullptr;
            getifaddrs(&interfaces);

            std::string local_ip;
            for (struct ifaddrs *iface = interfaces; iface != nullptr; iface = iface->ifa_next)
            {
                if (iface->ifa_addr->sa_family == AF_INET)
                { // Check for IPv4
                    char address[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &((struct sockaddr_in *)iface->ifa_addr)->sin_addr, address, sizeof(address));
                    if (strcmp(iface->ifa_name, "lo") != 0)
                    { // Exclude loopback interface
                        local_ip = address;
                        break; // Get the first non-loopback address
                    }
                }
            }

            freeifaddrs(interfaces);
            address_cache = local_ip;
        }

        return address_cache;
    }
}

#endif // UTILS_HPP