#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace osmv::ipc {

using RequestHandler = std::function<std::string(const std::string&)>;

class IpcServer {
public:
    IpcServer() = default;
    ~IpcServer();
    IpcServer(const IpcServer&) = delete;
    IpcServer& operator=(const IpcServer&) = delete;

    bool start(RequestHandler handler);
    void stop();

private:
    void run_loop();

    RequestHandler handler_;
    std::atomic<bool> running_{false};
    std::thread thread_;
#ifndef _WIN32
    int listen_fd_ = -1;
    std::string sock_path_;
#endif
};

// Sends a JSON request to the running daemon and waits for a JSON response.
// Returns true if communication succeeded.
bool send_request(const std::string& request_json,
                  std::string& response_json,
                  int timeout_ms = 500);

} // namespace osmv::ipc

