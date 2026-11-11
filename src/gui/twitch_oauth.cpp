#include "twitch_oauth.hpp"
#include "common/formatter.hpp"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstring>
#include <vector>

namespace osmv::gui {

namespace {
#ifdef _WIN32
using SockT = SOCKET;
constexpr SockT kInvalidSock = INVALID_SOCKET;
void close_sock(SockT s) {
    if (s != INVALID_SOCKET) closesocket(s);
}
struct WsaGuard {
    WsaGuard() {
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);
    }
    ~WsaGuard() { WSACleanup(); }
};
void ensure_wsa() {
    static WsaGuard guard;
}
#else
using SockT = int;
constexpr SockT kInvalidSock = -1;
void close_sock(SockT s) {
    if (s >= 0) close(s);
}
void ensure_wsa() {}
#endif

SockT bind_loopback_v4(uint16_t port) {
    SockT s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == kInvalidSock) return kInvalidSock;
#ifdef _WIN32
    BOOL excl = TRUE;
    setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&excl), sizeof(excl));
#else
    int reuse = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port);
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 || listen(s, 8) != 0) {
        close_sock(s);
        return kInvalidSock;
    }
    return s;
}

SockT bind_loopback_v6(uint16_t port) {
    SockT s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (s == kInvalidSock) return kInvalidSock;
    int v6only = 1;
    setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&v6only), sizeof(v6only));
#ifdef _WIN32
    BOOL excl = TRUE;
    setsockopt(s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&excl), sizeof(excl));
#else
    int reuse = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif
    sockaddr_in6 addr{};
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_loopback;
    addr.sin6_port = htons(port);
    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 || listen(s, 8) != 0) {
        close_sock(s);
        return kInvalidSock;
    }
    return s;
}

void send_http_response(SockT client, const char* status, const char* content_type, const std::string& body) {
    std::string hdr =
        std::string("HTTP/1.1 ") + status + "\r\n"
        "Content-Type: " + content_type + "\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n\r\n" + body;
    size_t sent = 0;
    while (sent < hdr.size()) {
        int n = send(client, hdr.data() + sent, static_cast<int>(hdr.size() - sent), 0);
        if (n <= 0) break;
        sent += static_cast<size_t>(n);
    }
}

std::string query_param(const std::string& query, const std::string& key) {
    std::string needle = key + "=";
    size_t pos = 0;
    while (pos < query.size()) {
        size_t amp = query.find('&', pos);
        std::string part = (amp == std::string::npos) ? query.substr(pos) : query.substr(pos, amp - pos);
        if (part.rfind(needle, 0) == 0) {
            return fmt::url_decode(part.substr(needle.size()));
        }
        if (amp == std::string::npos) break;
        pos = amp + 1;
    }
    return {};
}

std::string make_callback_html(bool is_fr) {
    const char* ok_title = is_fr ? "Twitch connect\xC3\xA9 \xC3\xA0 OSMV !" : "Twitch connected to OSMV!";
    const char* ok_msg = is_fr
        ? "Le token a \xC3\xA9t\xC3\xA9 r\xC3\xA9""cup\xC3\xA9r\xC3\xA9 automatiquement. Vous pouvez fermer cet onglet et revenir sur OSMV."
        : "Your token was captured automatically. You can close this tab and return to OSMV.";
    const char* err_title = is_fr ? "Autorisation non finalis\xC3\xA9""e" : "Authorization incomplete";
    const char* err_missing = is_fr ? "Aucun token trouv\xC3\xA9 dans la redirection." : "No token found in redirect URL.";

    return std::string(R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>OSMV - Twitch OAuth</title>
<style>
  body { margin:0; font-family: system-ui, -apple-system, sans-serif; background:#0e1117; color:#f1f5f9;
         display:flex; align-items:center; justify-content:center; min-height:100vh; }
  .card { background:#171c27; border:1px solid #293144; border-radius:14px; padding:32px 36px;
          max-width:460px; text-align:center; box-shadow:0 16px 48px rgba(0,0,0,0.45); }
  h1 { font-size:20px; margin:0 0 10px; color:#ffffff; }
  p { font-size:14px; color:#94a3b8; margin:0; line-height:1.5; }
  .ok { color:#22c55e; }
  .err { color:#ef4444; }
</style>
</head>
<body>
<div class="card">
  <h1 id="title">OSMV - Twitch OAuth</h1>
  <p id="msg">Completing authorization...</p>
</div>
<script>
(async () => {
  const titleEl = document.getElementById('title');
  const msgEl = document.getElementById('msg');
  const hash = window.location.hash ? window.location.hash.substring(1) : '';
  const search = window.location.search ? window.location.search.substring(1) : '';
  const hParams = new URLSearchParams(hash);
  const sParams = new URLSearchParams(search);
  const token = hParams.get('access_token');
  const err = hParams.get('error') || sParams.get('error') || sParams.get('error_description');

  if (window.history && window.history.replaceState) {
    window.history.replaceState(null, '', '/');
  }

  if (token) {
    try {
      const r = await fetch('/callback?token=' + encodeURIComponent(token));
      if (r.ok) {
        titleEl.className = 'ok';
        titleEl.textContent = ')HTML") + ok_title + R"HTML(';
        msgEl.textContent = ')HTML" + ok_msg + R"HTML(';
        return;
      }
    } catch (_) {}
  }

  titleEl.className = 'err';
  titleEl.textContent = ')HTML" + err_title + R"HTML(';
  msgEl.textContent = err ? ('Twitch error: ' + err) : ')HTML" + err_missing + R"HTML(';
  try { await fetch('/callback?error=' + encodeURIComponent(err || 'missing_token')); } catch (_) {}
})();
</script>
</body>
</html>)HTML";
}
} // namespace

TwitchOAuthListener::~TwitchOAuthListener() {
    stop();
}

bool TwitchOAuthListener::start(const std::string& lang) {
    stop();
    ensure_wsa();
    lang_ = lang;

    SockT s4 = bind_loopback_v4(80);
    SockT s6 = bind_loopback_v6(80);
    if (s4 == kInvalidSock && s6 == kInvalidSock) {
        return false;
    }

#ifdef _WIN32
    sock4_ = static_cast<uintptr_t>(s4);
    sock6_ = static_cast<uintptr_t>(s6);
#else
    sock4_ = s4;
    sock6_ = s6;
#endif

    {
        std::lock_guard<std::mutex> lock(res_mutex_);
        has_result_ = false;
        result_token_.clear();
        result_error_.clear();
    }

    running_.store(true);
    listening_.store(true);
    thread_ = std::thread(&TwitchOAuthListener::run_loop, this);
    return true;
}

void TwitchOAuthListener::stop() {
    running_.store(false);
    listening_.store(false);
    if (thread_.joinable()) {
        thread_.join();
    }

    SockT s4 = static_cast<SockT>(sock4_);
    SockT s6 = static_cast<SockT>(sock6_);
    if (s4 != kInvalidSock) {
        close_sock(s4);
        sock4_ = static_cast<decltype(sock4_)>(kInvalidSock);
    }
    if (s6 != kInvalidSock) {
        close_sock(s6);
        sock6_ = static_cast<decltype(sock6_)>(kInvalidSock);
    }
}

bool TwitchOAuthListener::poll_result(std::string& out_token, std::string& out_error) {
    std::lock_guard<std::mutex> lock(res_mutex_);
    if (!has_result_) return false;
    out_token = std::move(result_token_);
    out_error = std::move(result_error_);
    has_result_ = false;
    return true;
}

void TwitchOAuthListener::run_loop() {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(180);

    while (running_.load() && std::chrono::steady_clock::now() < deadline) {
        SockT s4 = static_cast<SockT>(sock4_);
        SockT s6 = static_cast<SockT>(sock6_);
        if (s4 == kInvalidSock && s6 == kInvalidSock) break;

        fd_set rfds;
        FD_ZERO(&rfds);
        SockT max_fd = 0;
        if (s4 != kInvalidSock) {
            FD_SET(s4, &rfds);
            if (s4 > max_fd) max_fd = s4;
        }
        if (s6 != kInvalidSock) {
            FD_SET(s6, &rfds);
            if (s6 > max_fd) max_fd = s6;
        }

        timeval tv{0, 200000}; // 200 ms
        int rc = select(static_cast<int>(max_fd) + 1, &rfds, nullptr, nullptr, &tv);
        if (rc <= 0 || !running_.load()) continue;

        auto handle_accept = [&](SockT srv) {
            if (srv == kInvalidSock || !FD_ISSET(srv, &rfds)) return false;
            SockT cli = accept(srv, nullptr, nullptr);
            if (cli == kInvalidSock) return false;

            char req_buf[4096];
            int n = recv(cli, req_buf, sizeof(req_buf) - 1, 0);
            if (n <= 0) {
                close_sock(cli);
                return false;
            }
            req_buf[n] = '\0';
            std::string req(req_buf, static_cast<size_t>(n));

            // Parse first line: "GET /path?query HTTP/1.1"
            std::string target = "/";
            auto sp1 = req.find(' ');
            if (sp1 != std::string::npos) {
                auto sp2 = req.find(' ', sp1 + 1);
                if (sp2 != std::string::npos) {
                    target = req.substr(sp1 + 1, sp2 - sp1 - 1);
                }
            }

            bool done = false;
            if (target.rfind("/favicon.ico", 0) == 0) {
                send_http_response(cli, "204 No Content", "text/plain", "");
            } else if (target.rfind("/callback", 0) == 0) {
                std::string q;
                auto qm = target.find('?');
                if (qm != std::string::npos) q = target.substr(qm + 1);
                std::string tok = query_param(q, "token");
                std::string err = query_param(q, "error");

                send_http_response(cli, "200 OK", "application/json", R"({"ok":true})");
                {
                    std::lock_guard<std::mutex> lock(res_mutex_);
                    result_token_ = std::move(tok);
                    result_error_ = std::move(err);
                    has_result_ = true;
                }
                done = true;
            } else {
                send_http_response(cli, "200 OK", "text/html; charset=utf-8",
                                   make_callback_html(lang_ == "fr"));
            }

            close_sock(cli);
            return done;
        };

        if (handle_accept(s4) || handle_accept(s6)) {
            break;
        }
    }

    listening_.store(false);
    running_.store(false);
    SockT s4 = static_cast<SockT>(sock4_);
    SockT s6 = static_cast<SockT>(sock6_);
    if (s4 != kInvalidSock) {
        close_sock(s4);
        sock4_ = static_cast<decltype(sock4_)>(kInvalidSock);
    }
    if (s6 != kInvalidSock) {
        close_sock(s6);
        sock6_ = static_cast<decltype(sock6_)>(kInvalidSock);
    }
}

} // namespace osmv::gui
