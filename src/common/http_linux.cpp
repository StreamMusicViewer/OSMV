#include "http.hpp"
#include "common/version.hpp"

#include <curl/curl.h>
#include <algorithm>

namespace osmv::http {

namespace {
constexpr size_t kMaxResponseBytes = 10 * 1024 * 1024;

struct CurlGlobalInit {
    CurlGlobalInit() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobalInit() { curl_global_cleanup(); }
};
CurlGlobalInit& ensure_curl_init() {
    static CurlGlobalInit inst;
    return inst;
}

size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* dst = static_cast<std::string*>(userdata);
    size_t n = size * nmemb;
    if (dst->size() + n > kMaxResponseBytes) return 0;
    dst->append(ptr, n);
    return n;
}
} // namespace

HttpResponse request(const std::string& method,
                     const std::string& url,
                     const std::vector<std::pair<std::string, std::string>>& headers,
                     const std::string& body,
                     int timeout_ms) {
    ensure_curl_init();
    HttpResponse res;
    if (url.empty()) {
        res.error = "Empty URL";
        return res;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        res.error = "curl_easy_init failed";
        return res;
    }

    struct curl_slist* hdr_list = nullptr;
    for (const auto& [k, v] : headers) {
        std::string h = k + ": " + v;
        hdr_list = curl_slist_append(hdr_list, h.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "OSMV/" OSMV_VERSION_STR);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(timeout_ms));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(timeout_ms));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res.body);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    if (hdr_list) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdr_list);
    }

    if (method != "GET") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
        if (!body.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
        }
    }

    CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK) {
        res.error = curl_easy_strerror(rc);
    } else {
        long code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        res.status_code = static_cast<int>(code);

        char* ct = nullptr;
        if (curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct) == CURLE_OK && ct) {
            res.content_type = ct;
        }
        res.ok = (res.status_code >= 200 && res.status_code < 300);
        if (!res.ok) {
            res.error = "HTTP " + std::to_string(res.status_code);
        }
    }

    if (hdr_list) curl_slist_free_all(hdr_list);
    curl_easy_cleanup(curl);
    return res;
}

} // namespace osmv::http
