#include "http.hpp"
#include <sstream>

namespace osmv::http {

std::vector<std::pair<std::string, std::string>> parse_header_lines(const std::string& raw) {
    std::vector<std::pair<std::string, std::string>> out;
    std::istringstream iss(raw);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto colon = line.find(':');
        if (colon == std::string::npos || colon == 0) continue;
        std::string k = line.substr(0, colon);
        std::string v = line.substr(colon + 1);
        auto trim = [](std::string& s) {
            size_t a = s.find_first_not_of(" \t");
            size_t b = s.find_last_not_of(" \t");
            if (a == std::string::npos) s.clear();
            else s = s.substr(a, b - a + 1);
        };
        trim(k);
        trim(v);
        if (!k.empty()) out.emplace_back(std::move(k), std::move(v));
    }
    return out;
}

} // namespace osmv::http

