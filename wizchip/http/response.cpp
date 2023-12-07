#include "wizchip/http/response.h"

using namespace Project;
using namespace Project::wizchip;

auto http::Response::get_head(etl::StringView key) const -> etl::StringView {
    for (auto &[key_, value] : head) if (key_ == key) {
        return value;
    }

    return nullptr;
}

auto http::Response::set_head(etl::StringView key, etl::StringView value) -> void {
    for (auto &[key_, value_] : head) if (not key_) {
        key_ = key;
        value_ = value;
    }
}