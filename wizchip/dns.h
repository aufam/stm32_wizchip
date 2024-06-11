#ifndef WIZCHIP_DNS_H
#define WIZCHIP_DNS_H

#include "wizchip/ethernet.h"
#include "etl/vector.h"
#include <string>

namespace Project::wizchip::dns {
    etl::Future<etl::Vector<uint8_t>> get_ip(std::string domain);
} 

#endif // WIZCHIP_DNS_H