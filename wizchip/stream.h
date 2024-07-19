#ifndef WIZCHIP_STREAM_H
#define WIZCHIP_STREAM_H

#include "etl/linked_list.h"
#include <functional>

namespace Project::wizchip {
    class Stream {
    public:
        using InRule = std::function<etl::Iter<const uint8_t*>()>;
        using OutRule = std::function<void(etl::Iter<const uint8_t*>)>;

        Stream& operator<<(etl::Iter<const uint8_t*> data);
        Stream& operator<<(InRule rule);
        Stream& operator>>(OutRule rule);
    
    private:
        etl::LinkedList<InRule> rules;
    };
}

#endif