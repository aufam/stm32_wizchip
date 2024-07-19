#include "wizchip/stream.h"
#include "etl/keywords.h"

using namespace wizchip;

Stream& Stream::operator<<(InRule rule) {
    rules << etl::move(rule);
    return *this;
}

Stream& Stream::operator<<(etl::Iter<const uint8_t*> data) {
    rules << [data]() { return data; };
    return *this;
}

Stream& Stream::operator>>(OutRule rule) {
    while (rules.len() > 0) {
        auto data = rules.front()();
        rules.pop_front();
        rule(data);
    }
    return *this;
}