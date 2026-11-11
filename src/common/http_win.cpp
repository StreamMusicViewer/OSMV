#include "http.hpp"
#include "os_utils.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>

#include <algorithm>

namespace osmv::http {

namespace {
constexpr size_t kMaxResponseBytes = 10 * 1024 * 1024;
}

HttpResponse request(const std::string& method,
                     const std::string& url,
                     const std::vector<std::pair<std::string, std::string>>& headers,
                     const std::string& body,
                     int timeout_ms) {
    HttpResponse res;
    if (url.empty()) {
        res.error = "Empty URL";
        return res;
    }

    std::wstring wurl = os::utf8_to_wide(url);
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);
    std::wstring host(256, L'\0');
    std::wstring path(2048, L'\0');
    uc.lpszHostName = host.data();
    uc.dwHostNameLength = static_cast<DWORD>(host.size());
    uc.lpszUrlPath = path.data();
    uc.dwUrlPathLength = static_cast<DWORD>(path.size());

    if (!WinHttpCrackUrl(wurl.c_str(), static_cast<DWORD>(wurl.size()), 0, &uc)) {
        res.error = "Invalid URL";
        return res;
    }
    host.resize(uc.dwHostNameLength);
    path.resize(uc.dwUrlPathLength);
    if (path.empty()) path = L"/";

    HINTERNET session = WinHttpOpen(L"OSMV/3.0",
                                    WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                    WINHTTP_NO_PROXY_NAME,
                                    WINHTTP_NO_PROXY_BYPASS,
                                    0);
    if (!session) {
        res.error = "WinHttpOpen failed";
        return res;
    }

    WinHttpSetTimeouts(session, timeout_ms, timeout_ms, timeout_ms, timeout_ms);
    DWORD redir = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(session, WINHTTP_OPTION_REDIRECT_POLICY, &redir, sizeof(redir));

    HINTERNET conn = WinHttpConnect(session, host.c_str(), uc.nPort, 0);
    if (!conn) {
        WinHttpCloseHandle(session);
        res.error = "WinHttpConnect failed";
        return res;
    }

    DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    std::wstring wmethod = os::utf8_to_wide(method);
    HINTERNET req = WinHttpOpenRequest(conn, wmethod.c_str(), path.c_str(),
                                       nullptr, WINHTTP_NO_REFERER,
                                       WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!req) {
        WinHttpCloseHandle(conn);
        WinHttpCloseHandle(session);
        res.error = "WinHttpOpenRequest failed";
        return res;
    }

    std::wstring hdrs;
    for (const auto& [k, v] : headers) {
        hdrs += os::utf8_to_wide(k) + L": " + os::utf8_to_wide(v) + L"\r\n";
    }
    if (!hdrs.empty()) {
        WinHttpAddRequestHeaders(req, hdrs.c_str(), static_cast<DWORD>(hdrs.size()),
                                 WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }

    BOOL ok = WinHttpSendRequest(
        req,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(body.data()),
        static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()),
        0);

    if (ok) ok = WinHttpReceiveResponse(req, nullptr);
    if (!ok) {
        res.error = "HTTP request failed (" + std::to_string(GetLastError()) + ")";
        WinHttpCloseHandle(req);
        WinHttpCloseHandle(conn);
        WinHttpCloseHandle(session);
        return res;
    }

    DWORD status_code = 0;
    DWORD status_size = sizeof(status_code);
    if (WinHttpQueryHeaders(req,
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX,
                            &status_code,
                            &status_size,
                            WINHTTP_NO_HEADER_INDEX)) {
        res.status_code = static_cast<int>(status_code);
    }

    DWORD ct_len = 0;
    WinHttpQueryHeaders(req, WINHTTP_QUERY_CONTENT_TYPE, WINHTTP_HEADER_NAME_BY_INDEX,
                        nullptr, &ct_len, WINHTTP_NO_HEADER_INDEX);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER && ct_len > 0) {
        std::wstring ct(ct_len / sizeof(wchar_t), L'\0');
        if (WinHttpQueryHeaders(req, WINHTTP_QUERY_CONTENT_TYPE, WINHTTP_HEADER_NAME_BY_INDEX,
                                ct.data(), &ct_len, WINHTTP_NO_HEADER_INDEX)) {
            while (!ct.empty() && ct.back() == L'\0') ct.pop_back();
            res.content_type = os::wide_to_utf8(ct);
        }
    }

    char buf[8192];
    DWORD avail = 0;
    while (WinHttpQueryDataAvailable(req, &avail) && avail > 0) {
        while (avail > 0) {
            DWORD to_read = std::min<DWORD>(avail, sizeof(buf));
            DWORD read = 0;
            if (!WinHttpReadData(req, buf, to_read, &read) || read == 0) break;
            if (res.body.size() + read > kMaxResponseBytes) {
                res.error = "Response too large";
                WinHttpCloseHandle(req);
                WinHttpCloseHandle(conn);
                WinHttpCloseHandle(session);
                return res;
            }
            res.body.append(buf, read);
            avail -= read;
        }
    }

    WinHttpCloseHandle(req);
    WinHttpCloseHandle(conn);
    WinHttpCloseHandle(session);

    res.ok = (res.status_code >= 200 && res.status_code < 300);
    if (!res.ok && res.error.empty()) {
        res.error = "HTTP " + std::to_string(res.status_code);
    }
    return res;
}

} // namespace osmv::http
