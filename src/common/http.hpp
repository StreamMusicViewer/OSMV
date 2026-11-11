#pragma once

#include <string>
#include <utility>
#include <vector>

namespace osmv::http {

struct HttpResponse {
    bool ok = false;
    int status_code = 0;
    std::string content_type;
    std::string body;
    std::string error;
};

HttpResponse request(const std::string& method,
                     const std::string& url,
                     const std::vector<std::pair<std::string, std::string>>& headers = {},
                     const std::string& body = "",
                     int timeout_ms = 6000);

inline HttpResponse get(const std::string& url,
                        const std::vector<std::pair<std::string, std::string>>& headers = {},
                        int timeout_ms = 6000) {
    return request("GET", url, headers, "", timeout_ms);
}

std::vector<std::pair<std::string, std::string>> parse_header_lines(const std::string& raw);

} // namespace osmv::http

