#include "wizchip/url.h"
#include "etl/string.h"

using namespace Project;
using namespace Project::wizchip;

static auto parse_query(etl::StringView sv) -> etl::UnorderedMap<std::string, std::string>;
static auto sv_percent_encoded_to_string(etl::StringView sv) -> std::string;
static auto sv_to_string(etl::StringView sv) { return std::string(sv.data(), sv.len()); }

static constexpr int hex_to_int(char ch) {
    return (ch >= '0' && ch <= '9') ? ch - '0' :
           (ch >= 'A' && ch <= 'F') ? ch - 'A' + 10 :
           (ch >= 'a' && ch <= 'f') ? ch - 'a' + 10 : -1;
}

URL::URL(etl::StringView sv) : URL(sv_to_string(sv)) {}

URL::URL(std::string s) : url(etl::move(s)) {
    auto sv = etl::string_view(url.data(), url.size());

    if (sv.len() == 0)
        return;

    if (auto dom = sv.find("://"); dom < sv.len()) {
        sv = sv.substr(dom + 3, sv.len() - (dom + 3));
    }
    
    struct { int start; int stop; bool found; } dom = {}, path = {}, que = {}, frag = {};
    int end = sv.len() - 1;

    for (int i = 0; i < end; ++i) {
        char ch = sv[i];

        if (not dom.found and ch == '/') {
            dom.start = 0;
            dom.stop = i;
            dom.found = true;
        }
        if (not path.found and ch == '/') {
            path.start = i;
            path.stop = sv.len();
            path.found = true;
        }
        if (not que.found and ch == '?') {
            que.start = i + 1;
            que.stop = sv.len();
            que.found = true;

            if (path.found) {
                path.stop = i;
            }
        } 
        if (not frag.found and ch == '#') {
            frag.start = i + 1;
            frag.stop = sv.len();

            // found '#' before '?'
            if (not que.found) {
                que = frag;
                path.stop = i;
            } 
            // found '#' after '?'
            else {
                que.stop = i;
            }
        } 
    }

    this->host = sv_to_string(sv.substr(dom.start, dom.stop - dom.start));
    if (path.found) {
        this->path = sv_to_string(sv.substr(path.start, path.stop - path.start));
        this->full_path = sv_to_string(sv.substr(path.start, sv.len() - path.start));
    } else {
        this->path = "/";
        this->full_path = "/";
    }
    this->path = sv_to_string(sv.substr(path.start, path.stop - path.start));
    this->queries = parse_query(sv.substr(que.start, que.stop - que.start));
    this->fragment = sv_to_string(sv.substr(frag.start, frag.stop - frag.start));
}

auto sv_percent_encoded_to_string(etl::StringView sv) -> std::string {
    std::string res;
    res.reserve(sv.len() + 1);

    for (size_t i = 0; i < sv.len(); ++i) {
        if (sv[i] != '%') {
            res += sv[i];
            continue;
        }

        int a = hex_to_int(sv[i + 1]);
        int b = hex_to_int(sv[i + 2]);
        if (a >= 0 and b >= 0) {
            res += static_cast<char>(a << 4 | b);
            i += 2;
        }
    }

    return res;
}

auto parse_query(etl::StringView sv) -> etl::UnorderedMap<std::string, std::string> {
    auto res = etl::UnorderedMap<std::string, std::string>();

    for (auto kv : sv.split<16>("&")) {
        auto [key, value] = kv.split<2>("=");
        res[sv_percent_encoded_to_string(key)] = sv_percent_encoded_to_string(value);
    }

    return res;
}

auto wizchip::literals::operator ""_url(const char* text, size_t n) -> URL {
    return URL(etl::string_view(text, n));
}