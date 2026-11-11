#include "ipc.hpp"
#include "os_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <cstdlib>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace osmv::ipc {

namespace {
constexpr uint32_t kMaxMsgSize = 512 * 1024;

#ifdef _WIN32
constexpr wchar_t kPipeName[] = L"\\\\.\\pipe\\osmv-ipc-v3";

bool write_exact_win(HANDLE h, const void* data, DWORD len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    DWORD sent = 0;
    while (sent < len) {
        DWORD n = 0;
        if (!WriteFile(h, p + sent, len - sent, &n, nullptr) || n == 0) return false;
        sent += n;
    }
    return true;
}

bool read_exact_win(HANDLE h, void* data, DWORD len) {
    uint8_t* p = static_cast<uint8_t*>(data);
    DWORD got = 0;
    while (got < len) {
        DWORD n = 0;
        if (!ReadFile(h, p + got, len - got, &n, nullptr) || n == 0) return false;
        got += n;
    }
    return true;
}

bool send_frame_win(HANDLE h, const std::string& msg) {
    uint32_t len = static_cast<uint32_t>(std::min<size_t>(msg.size(), kMaxMsgSize));
    return write_exact_win(h, &len, 4) && (len == 0 || write_exact_win(h, msg.data(), len));
}

bool recv_frame_win(HANDLE h, std::string& out) {
    uint32_t len = 0;
    if (!read_exact_win(h, &len, 4) || len > kMaxMsgSize) return false;
    out.resize(len);
    return len == 0 || read_exact_win(h, out.data(), len);
}
#else
std::string unix_sock_path() {
    const char* xdg = std::getenv("XDG_RUNTIME_DIR");
    if (xdg && xdg[0]) {
        return std::string(xdg) + "/osmv-ipc-v3.sock";
    }
    return "/tmp/osmv-ipc-v3-" + std::to_string(static_cast<long>(getuid())) + ".sock";
}

bool write_exact_posix(int fd, const void* data, size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, p + sent, len - sent, MSG_NOSIGNAL);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            return false;
        }
        sent += static_cast<size_t>(n);
    }
    return true;
}

bool read_exact_posix(int fd, void* data, size_t len) {
    uint8_t* p = static_cast<uint8_t*>(data);
    size_t got = 0;
    while (got < len) {
        ssize_t n = recv(fd, p + got, len - got, 0);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            return false;
        }
        got += static_cast<size_t>(n);
    }
    return true;
}

bool send_frame_posix(int fd, const std::string& msg) {
    uint32_t len = static_cast<uint32_t>(std::min<size_t>(msg.size(), kMaxMsgSize));
    return write_exact_posix(fd, &len, 4) && (len == 0 || write_exact_posix(fd, msg.data(), len));
}

bool recv_frame_posix(int fd, std::string& out) {
    uint32_t len = 0;
    if (!read_exact_posix(fd, &len, 4) || len > kMaxMsgSize) return false;
    out.resize(len);
    return len == 0 || read_exact_posix(fd, out.data(), len);
}
#endif
} // namespace

IpcServer::~IpcServer() {
    stop();
}

bool IpcServer::start(RequestHandler handler) {
    if (running_.load()) return true;
    handler_ = std::move(handler);

#ifndef _WIN32
    sock_path_ = unix_sock_path();
    unlink(sock_path_.c_str());
    listen_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd_ < 0) return false;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (sock_path_.size() >= sizeof(addr.sun_path)) {
        close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }
    std::strncpy(addr.sun_path, sock_path_.c_str(), sizeof(addr.sun_path) - 1);
    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0 ||
        listen(listen_fd_, 8) != 0) {
        close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }
#endif

    running_.store(true);
    thread_ = std::thread(&IpcServer::run_loop, this);
    return true;
}

void IpcServer::stop() {
    if (!running_.exchange(false)) return;

#ifdef _WIN32
    // Poke the pipe if waiting on ConnectNamedPipe
    HANDLE wake = CreateFileW(kPipeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                              OPEN_EXISTING, 0, nullptr);
    if (wake != INVALID_HANDLE_VALUE) {
        CloseHandle(wake);
    }
#else
    if (listen_fd_ >= 0) {
        shutdown(listen_fd_, SHUT_RDWR);
    }
#endif

    if (thread_.joinable()) {
        thread_.join();
    }

#ifndef _WIN32
    if (listen_fd_ >= 0) {
        close(listen_fd_);
        listen_fd_ = -1;
    }
    if (!sock_path_.empty()) {
        unlink(sock_path_.c_str());
    }
#endif
}

void IpcServer::run_loop() {
#ifdef _WIN32
    while (running_.load()) {
        HANDLE pipe = CreateNamedPipeW(
            kPipeName,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            65536,
            65536,
            500,
            nullptr);

        if (pipe == INVALID_HANDLE_VALUE) {
            os::sleep_ms(50);
            continue;
        }

        BOOL connected = ConnectNamedPipe(pipe, nullptr);
        if (!connected && GetLastError() == ERROR_PIPE_CONNECTED) {
            connected = TRUE;
        }

        if (connected && running_.load()) {
            std::string req;
            if (recv_frame_win(pipe, req) && handler_) {
                std::string resp = handler_(req);
                send_frame_win(pipe, resp);
                FlushFileBuffers(pipe);
            }
        }
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
#else
    while (running_.load()) {
        pollfd pfd{};
        pfd.fd = listen_fd_;
        pfd.events = POLLIN;
        int rc = poll(&pfd, 1, 150);
        if (rc <= 0 || !running_.load()) continue;
        int client = accept(listen_fd_, nullptr, nullptr);
        if (client < 0) continue;

        timeval tv{};
        tv.tv_sec = 1;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        std::string req;
        if (recv_frame_posix(client, req) && handler_) {
            std::string resp = handler_(req);
            send_frame_posix(client, resp);
        }
        close(client);
    }
#endif
}

bool send_request(const std::string& request_json,
                  std::string& response_json,
                  int timeout_ms) {
#ifdef _WIN32
    HANDLE pipe = INVALID_HANDLE_VALUE;
    for (int attempt = 0; attempt < 3; ++attempt) {
        pipe = CreateFileW(kPipeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                           OPEN_EXISTING, 0, nullptr);
        if (pipe != INVALID_HANDLE_VALUE) break;
        if (GetLastError() != ERROR_PIPE_BUSY) return false;
        if (!WaitNamedPipeW(kPipeName, static_cast<DWORD>(timeout_ms))) return false;
    }
    if (pipe == INVALID_HANDLE_VALUE) return false;

    bool ok = send_frame_win(pipe, request_json) && recv_frame_win(pipe, response_json);
    CloseHandle(pipe);
    return ok;
#else
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return false;

    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::string sp = unix_sock_path();
    if (sp.size() >= sizeof(addr.sun_path)) {
        close(fd);
        return false;
    }
    std::strncpy(addr.sun_path, sp.c_str(), sizeof(addr.sun_path) - 1);
    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close(fd);
        return false;
    }

    bool ok = send_frame_posix(fd, request_json) && recv_frame_posix(fd, response_json);
    close(fd);
    return ok;
#endif
}

} // namespace osmv::ipc
