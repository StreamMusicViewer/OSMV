#include "tray.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

#include <atomic>
#include <mutex>

namespace osmv::daemon {

namespace {
constexpr UINT WM_TRAYICON = WM_APP + 10;
constexpr UINT WM_QUIT_DAEMON = WM_APP + 11;
constexpr UINT ID_TRAY_OPEN = 1001;
constexpr UINT ID_TRAY_QUIT = 1002;

class WindowsTrayController final : public TrayController {
public:
    ~WindowsTrayController() override {
        cleanup();
    }

    bool init(const TrayCallbacks& callbacks) override {
        callbacks_ = callbacks;
        HINSTANCE hinst = GetModuleHandleW(nullptr);

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = &WindowsTrayController::wnd_proc;
        wc.hInstance = hinst;
        wc.lpszClassName = L"OSMV_TrayHiddenWindow";
        RegisterClassExW(&wc);

        hwnd_ = CreateWindowExW(
            0, wc.lpszClassName, L"OSMV Tray", 0,
            0, 0, 0, 0, nullptr, nullptr, hinst, this);
        if (!hwnd_) return false;

        msg_taskbar_created_ = RegisterWindowMessageW(L"TaskbarCreated");
        add_tray_icon();
        return true;
    }

    void set_language(const std::string& lang) override {
        std::lock_guard<std::mutex> lock(lang_mutex_);
        is_fr_ = (lang == "fr");
    }

    void run_event_loop() override {
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        cleanup();
    }

    void request_quit() override {
        if (hwnd_) {
            PostMessageW(hwnd_, WM_QUIT_DAEMON, 0, 0);
        }
    }

private:
    void add_tray_icon() {
        if (!hwnd_) return;
        NOTIFYICONDATAW nid{};
        nid.cbSize = sizeof(nid);
        nid.hWnd = hwnd_;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        nid.hIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(101));
        if (!nid.hIcon) {
            nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        }
        wcscpy_s(nid.szTip, L"OSMV - OBS Stream Music Viewer");
        Shell_NotifyIconW(NIM_ADD, &nid);
        icon_added_ = true;
    }

    void cleanup() {
        if (icon_added_ && hwnd_) {
            NOTIFYICONDATAW nid{};
            nid.cbSize = sizeof(nid);
            nid.hWnd = hwnd_;
            nid.uID = 1;
            Shell_NotifyIconW(NIM_DELETE, &nid);
            icon_added_ = false;
        }
        if (hwnd_) {
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
        }
    }

    void show_context_menu() {
        HMENU menu = CreatePopupMenu();
        if (!menu) return;

        bool fr = false;
        {
            std::lock_guard<std::mutex> lock(lang_mutex_);
            fr = is_fr_;
        }

        AppendMenuW(menu, MF_STRING, ID_TRAY_OPEN,
                    fr ? L"Afficher la configuration" : L"Show Configuration");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, ID_TRAY_QUIT,
                    fr ? L"Quitter OSMV" : L"Quit OSMV");

        POINT pt{};
        GetCursorPos(&pt);
        SetForegroundWindow(hwnd_);
        UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                                  pt.x, pt.y, 0, hwnd_, nullptr);
        PostMessageW(hwnd_, WM_NULL, 0, 0);
        DestroyMenu(menu);

        if (cmd == ID_TRAY_OPEN && callbacks_.on_show_gui) {
            callbacks_.on_show_gui();
        } else if (cmd == ID_TRAY_QUIT && callbacks_.on_quit) {
            callbacks_.on_quit();
        }
    }

    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        auto* self = reinterpret_cast<WindowsTrayController*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
            self = static_cast<WindowsTrayController*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            return DefWindowProcW(hwnd, msg, wp, lp);
        }

        if (self) {
            if (msg == self->msg_taskbar_created_ && self->msg_taskbar_created_ != 0) {
                self->add_tray_icon();
                return 0;
            }
            switch (msg) {
                case WM_TRAYICON:
                    if (lp == WM_LBUTTONDBLCLK || lp == WM_LBUTTONUP) {
                        if (self->callbacks_.on_show_gui) self->callbacks_.on_show_gui();
                    } else if (lp == WM_RBUTTONUP || lp == WM_CONTEXTMENU) {
                        self->show_context_menu();
                    }
                    return 0;
                case WM_QUIT_DAEMON:
                    PostQuitMessage(0);
                    return 0;
                case WM_DESTROY:
                    self->icon_added_ = false;
                    return 0;
            }
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    HWND hwnd_ = nullptr;
    UINT msg_taskbar_created_ = 0;
    bool icon_added_ = false;
    TrayCallbacks callbacks_;
    std::mutex lang_mutex_;
    bool is_fr_ = true;
};
} // namespace

std::unique_ptr<TrayController> create_tray_controller() {
    return std::make_unique<WindowsTrayController>();
}

} // namespace osmv::daemon

