#include "dynamic_files.hpp"
#include "common/http.hpp"
#include "common/settings.hpp"

#include <algorithm>
#include <cctype>

namespace osmv::daemon {

namespace {
std::string trim_copy(std::string s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

bool starts_with_icase(const std::string& s, const char* prefix) {
    size_t n = std::char_traits<char>::length(prefix);
    if (s.size() < n) return false;
    for (size_t i = 0; i < n; ++i) {
        if (std::tolower(static_cast<unsigned char>(s[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i]))) {
            return false;
        }
    }
    return true;
}

std::string build_sig(const DynamicFilesSettings& settings) {
    std::string sig = settings.enabled ? "1|" : "0|";
    for (const auto& r : settings.rules) {
        sig += (r.enabled ? "1:" : "0:") + r.filename + ":" + r.url + ":" + r.type + ":" +
               std::to_string(r.interval_sec) + ":" + r.headers + "|";
    }
    return sig;
}
} // namespace

void DynamicFilesRunner::sync_rules(const DynamicFilesSettings& settings,
                                    const std::filesystem::path& out_dir) {
    std::string sig = build_sig(settings);
    if (sig == last_config_sig_ && runtimes_.size() == settings.rules.size()) return;
    last_config_sig_ = std::move(sig);
    runtimes_.clear();
    runtimes_.resize(settings.rules.size());
    for (size_t i = 0; i < settings.rules.size(); ++i) {
        runtimes_[i].status.filename = settings.rules[i].filename;
        runtimes_[i].status.status = (!settings.enabled || !settings.rules[i].enabled) ? "Disabled" : "Pending";
        runtimes_[i].status.ok = true;
        runtimes_[i].next_run = std::chrono::steady_clock::time_point{};
    }
    (void)out_dir;
}

void DynamicFilesRunner::tick(const DynamicFilesSettings& settings,
                              const std::filesystem::path& out_dir) {
    sync_rules(settings, out_dir);
    if (!settings.enabled) return;

    auto now = std::chrono::steady_clock::now();
    for (size_t i = 0; i < settings.rules.size(); ++i) {
        const auto& r = settings.rules[i];
        auto& rt = runtimes_[i];
        rt.status.filename = r.filename;
        if (!r.enabled || r.url.empty() || r.filename.empty()) {
            rt.status.status = r.enabled ? "Missing URL" : "Disabled";
            rt.status.ok = true;
            continue;
        }
        if (now < rt.next_run) continue;

        rt.next_run = now + std::chrono::seconds(std::clamp(r.interval_sec, 5, 86400));

        auto hdrs = http::parse_header_lines(r.headers);
        auto resp = http::get(r.url, hdrs, 6000);
        if (!resp.ok) {
            rt.status.ok = false;
            rt.status.status = resp.error.empty() ? "Request failed" : resp.error;
            continue;
        }

        std::filesystem::path target = out_dir / r.filename;
        if (r.type == "image") {
            if (starts_with_icase(resp.content_type, "image/")) {
                if (cfg::write_file_bytes(target, resp.body.data(), resp.body.size())) {
                    rt.status.ok = true;
                    rt.status.status = "OK (" + std::to_string(resp.body.size() / 1024 + 1) + " KB)";
                    rt.status.preview = r.filename;
                } else {
                    rt.status.ok = false;
                    rt.status.status = "Write error";
                }
            } else {
                // Snaz image mode: endpoint may return an image URL in plain text
                std::string img_url = trim_copy(resp.body);
                if (starts_with_icase(img_url, "http://") || starts_with_icase(img_url, "https://")) {
                    auto img_resp = http::get(img_url, {}, 7000);
                    if (img_resp.ok && !img_resp.body.empty()) {
                        if (cfg::write_file_bytes(target, img_resp.body.data(), img_resp.body.size())) {
                            rt.status.ok = true;
                            rt.status.status = "OK (" + std::to_string(img_resp.body.size() / 1024 + 1) + " KB)";
                            rt.status.preview = img_url;
                        } else {
                            rt.status.ok = false;
                            rt.status.status = "Write error";
                        }
                    } else {
                        rt.status.ok = false;
                        rt.status.status = img_resp.error.empty() ? "Image fetch failed" : img_resp.error;
                    }
                } else {
                    rt.status.ok = false;
                    rt.status.status = "Expected image or URL";
                }
            }
        } else {
            if (cfg::write_file_text(target, resp.body)) {
                rt.status.ok = true;
                rt.status.status = "OK (" + std::to_string(resp.status_code) + ")";
                std::string prev = trim_copy(resp.body);
                if (prev.size() > 80) prev = prev.substr(0, 80) + "...";
                rt.status.preview = std::move(prev);
            } else {
                rt.status.ok = false;
                rt.status.status = "Write error";
            }
        }
    }
}

std::vector<DynamicRuleStatus> DynamicFilesRunner::statuses() const {
    std::vector<DynamicRuleStatus> out;
    out.reserve(runtimes_.size());
    for (const auto& rt : runtimes_) {
        out.push_back(rt.status);
    }
    return out;
}

} // namespace osmv::daemon

