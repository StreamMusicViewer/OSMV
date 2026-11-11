#include "gui_backend.hpp"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <dxgi.h>

#include <chrono>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace osmv::gui {

namespace {
class Win32Dx11Backend final : public GuiBackend {
public:
    ~Win32Dx11Backend() override { shutdown(); }

    bool init(const char* title, int width, int height, bool dark_titlebar) override {
        ImGui_ImplWin32_EnableDpiAwareness();

        HINSTANCE hinst = GetModuleHandleW(nullptr);
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_CLASSDC;
        wc.lpfnWndProc = &Win32Dx11Backend::wnd_proc;
        wc.hInstance = hinst;
        wc.hIcon = LoadIconW(hinst, MAKEINTRESOURCEW(101));
        wc.hIconSm = LoadIconW(hinst, MAKEINTRESOURCEW(101));
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = L"OSMV_GuiWindowClass";
        RegisterClassExW(&wc);
        class_atom_ = true;

        float dpi_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(
            MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));
        scale_ = dpi_scale > 0.5f ? dpi_scale : 1.0f;

        int win_w = static_cast<int>(width * scale_);
        int win_h = static_cast<int>(height * scale_);

        int wlen = MultiByteToWideChar(CP_UTF8, 0, title, -1, nullptr, 0);
        std::wstring wtitle(static_cast<size_t>(wlen > 0 ? wlen : 1), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle.data(), wlen);

        RECT wr{0, 0, win_w, win_h};
        AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

        int sw = GetSystemMetrics(SM_CXSCREEN);
        int sh = GetSystemMetrics(SM_CYSCREEN);
        int x = (sw - (wr.right - wr.left)) / 2;
        int y = (sh - (wr.bottom - wr.top)) / 2;

        hwnd_ = CreateWindowExW(
            0, wc.lpszClassName, wtitle.c_str(),
            WS_OVERLAPPEDWINDOW,
            x, y, wr.right - wr.left, wr.bottom - wr.top,
            nullptr, nullptr, hinst, this);
        if (!hwnd_) return false;

        if (!create_device_d3d()) {
            cleanup_device_d3d();
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
            return false;
        }

        set_dark_titlebar(dark_titlebar);
        ShowWindow(hwnd_, SW_SHOWDEFAULT);
        UpdateWindow(hwnd_);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr; // Do not generate imgui.ini clutter
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui_ImplWin32_Init(hwnd_);
        ImGui_ImplDX11_Init(device_, ctx_);

        last_interaction_ = std::chrono::steady_clock::now();
        return true;
    }

    void shutdown() override {
        if (initialized_imgui_) {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            initialized_imgui_ = false;
        } else if (ImGui::GetCurrentContext()) {
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
        }
        cleanup_device_d3d();
        if (hwnd_) {
            DestroyWindow(hwnd_);
            hwnd_ = nullptr;
        }
        if (class_atom_) {
            UnregisterClassW(L"OSMV_GuiWindowClass", GetModuleHandleW(nullptr));
            class_atom_ = false;
        }
    }

    bool should_close() const override { return should_close_; }

    void wait_events(double timeout_sec) override {
        DWORD ms = timeout_sec <= 0.0 ? 0 : static_cast<DWORD>(timeout_sec * 1000.0);
        if (ms > 0) {
            MsgWaitForMultipleObjects(0, nullptr, FALSE, ms, QS_ALLINPUT);
        }
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) should_close_ = true;
        }
    }

    bool begin_frame() override {
        if (should_close_) return false;

        if (swapchain_occluded_ && swapchain_->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            Sleep(25);
            return false;
        }
        swapchain_occluded_ = false;

        if (resize_w_ != 0 && resize_h_ != 0) {
            cleanup_render_target();
            swapchain_->ResizeBuffers(0, resize_w_, resize_h_, DXGI_FORMAT_UNKNOWN, 0);
            resize_w_ = resize_h_ = 0;
            create_render_target();
        }

        if (IsIconic(hwnd_)) {
            Sleep(50);
            return false;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        initialized_imgui_ = true;
        return true;
    }

    void end_frame(float clear_r, float clear_g, float clear_b) override {
        ImGui::Render();
        const float cc[4] = {clear_r, clear_g, clear_b, 1.0f};
        ctx_->OMSetRenderTargets(1, &rtv_, nullptr);
        ctx_->ClearRenderTargetView(rtv_, cc);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = swapchain_->Present(1, 0);
        swapchain_occluded_ = (hr == DXGI_STATUS_OCCLUDED);
    }

    void set_dark_titlebar(bool dark) override {
        if (!hwnd_) return;
        BOOL val = dark ? TRUE : FALSE;
        // DWMWA_USE_IMMERSIVE_DARK_MODE (20 on Win10 20H1+ / Win11)
        DwmSetWindowAttribute(hwnd_, 20, &val, sizeof(val));
    }

    float content_scale() const override { return scale_; }

    bool has_active_interaction() const override {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - last_interaction_).count();
        return elapsed < 450;
    }

    ImTextureID create_rgba_texture(int width, int height, const uint8_t* rgba) override {
        if (!device_ || width <= 0 || height <= 0 || !rgba) return 0;

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(width);
        desc.Height = static_cast<UINT>(height);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA sub{};
        sub.pSysMem = rgba;
        sub.SysMemPitch = static_cast<UINT>(width * 4);

        ID3D11Texture2D* tex = nullptr;
        if (FAILED(device_->CreateTexture2D(&desc, &sub, &tex)) || !tex) return 0;

        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
        srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srv_desc.Texture2D.MipLevels = 1;

        ID3D11ShaderResourceView* srv = nullptr;
        HRESULT hr = device_->CreateShaderResourceView(tex, &srv_desc, &srv);
        tex->Release();
        if (FAILED(hr) || !srv) return 0;

        return static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(srv));
    }

    void destroy_texture(ImTextureID tex) override {
        if (!tex) return;
        auto* srv = reinterpret_cast<ID3D11ShaderResourceView*>(static_cast<uintptr_t>(tex));
        srv->Release();
    }

private:
    bool create_device_d3d() {
        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount = 2;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd_;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        D3D_FEATURE_LEVEL level{};
        const D3D_FEATURE_LEVEL levels[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
        HRESULT hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            levels, 2, D3D11_SDK_VERSION,
            &sd, &swapchain_, &device_, &level, &ctx_);
        if (hr == DXGI_ERROR_UNSUPPORTED) {
            hr = D3D11CreateDeviceAndSwapChain(
                nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
                levels, 2, D3D11_SDK_VERSION,
                &sd, &swapchain_, &device_, &level, &ctx_);
        }
        if (FAILED(hr)) return false;
        create_render_target();
        return true;
    }

    void cleanup_device_d3d() {
        cleanup_render_target();
        if (swapchain_) { swapchain_->Release(); swapchain_ = nullptr; }
        if (ctx_) { ctx_->Release(); ctx_ = nullptr; }
        if (device_) { device_->Release(); device_ = nullptr; }
    }

    void create_render_target() {
        ID3D11Texture2D* back = nullptr;
        if (SUCCEEDED(swapchain_->GetBuffer(0, IID_PPV_ARGS(&back))) && back) {
            device_->CreateRenderTargetView(back, nullptr, &rtv_);
            back->Release();
        }
    }

    void cleanup_render_target() {
        if (rtv_) { rtv_->Release(); rtv_ = nullptr; }
    }

    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        auto* self = reinterpret_cast<Win32Dx11Backend*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
            self = static_cast<Win32Dx11Backend*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }

        if (self && ((msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) ||
                     (msg >= WM_KEYFIRST && msg <= WM_KEYLAST) ||
                     msg == WM_SIZE)) {
            self->last_interaction_ = std::chrono::steady_clock::now();
        }

        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)) {
            return TRUE;
        }

        switch (msg) {
            case WM_GETMINMAXINFO: {
                if (self) {
                    auto* mmi = reinterpret_cast<MINMAXINFO*>(lp);
                    mmi->ptMinTrackSize.x = static_cast<LONG>(740 * self->scale_);
                    mmi->ptMinTrackSize.y = static_cast<LONG>(480 * self->scale_);
                }
                return 0;
            }
            case WM_SIZE:
                if (self && wp != SIZE_MINIMIZED) {
                    self->resize_w_ = LOWORD(lp);
                    self->resize_h_ = HIWORD(lp);
                }
                return 0;
            case WM_DPICHANGED:
                if (self) {
                    self->scale_ = static_cast<float>(HIWORD(wp)) / 96.0f;
                    const RECT* suggested = reinterpret_cast<RECT*>(lp);
                    SetWindowPos(hwnd, nullptr, suggested->left, suggested->top,
                                 suggested->right - suggested->left,
                                 suggested->bottom - suggested->top,
                                 SWP_NOZORDER | SWP_NOACTIVATE);
                }
                return 0;
            case WM_SYSCOMMAND:
                if ((wp & 0xfff0) == SC_KEYMENU) return 0;
                break;
            case WM_CLOSE:
                if (self) self->should_close_ = true;
                DestroyWindow(hwnd);
                return 0;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    HWND hwnd_ = nullptr;
    bool class_atom_ = false;
    bool initialized_imgui_ = false;
    bool should_close_ = false;
    bool swapchain_occluded_ = false;
    UINT resize_w_ = 0;
    UINT resize_h_ = 0;
    float scale_ = 1.0f;
    std::chrono::steady_clock::time_point last_interaction_{};

    ID3D11Device* device_ = nullptr;
    ID3D11DeviceContext* ctx_ = nullptr;
    IDXGISwapChain* swapchain_ = nullptr;
    ID3D11RenderTargetView* rtv_ = nullptr;
};
} // namespace

std::unique_ptr<GuiBackend> create_gui_backend() {
    return std::make_unique<Win32Dx11Backend>();
}

} // namespace osmv::gui

