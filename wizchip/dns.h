#ifndef WIZCHIP_DNS_H
#define WIZCHIP_DNS_H

#include "etl/vector.h"
#include "etl/future.h"
#include <string>

namespace Project::wizchip::dns {
    etl::Future<etl::Vector<uint8_t>> get_ip(const std::string& domain);
} 

#endif // WIZCHIP_DNS_H