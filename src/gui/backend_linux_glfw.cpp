#include "gui_backend.hpp"
#include "common/embedded_logo.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <chrono>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

namespace osmv::gui {

namespace {
class LinuxGlfwBackend final : public GuiBackend {
public:
    ~LinuxGlfwBackend() override { shutdown(); }

    bool init(const char* title, int width, int height, bool dark_titlebar) override {
        (void)dark_titlebar;
        if (!glfwInit()) return false;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

        window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!window_) {
            glfwTerminate();
            return false;
        }
        glfwSetWindowSizeLimits(window_, 740, 480, GLFW_DONT_CARE, GLFW_DONT_CARE);

        // Set window icon from embedded PNG
        int iw = 0, ih = 0, ich = 0;
        uint8_t* pixels = stbi_load_from_memory(
            assets::kLogoPng, static_cast<int>(assets::kLogoPngSize), &iw, &ih, &ich, 4);
        if (pixels) {
            GLFWimage img{iw, ih, pixels};
            glfwSetWindowIcon(window_, 1, &img);
            stbi_image_free(pixels);
        }

        glfwMakeContextCurrent(window_);
        glfwSwapInterval(1);

        float xscale = 1.0f, yscale = 1.0f;
        glfwGetWindowContentScale(window_, &xscale, &yscale);
        scale_ = xscale > 0.5f ? xscale : 1.0f;

        glfwSetWindowUserPointer(window_, this);
        glfwSetCursorPosCallback(window_, [](GLFWwindow* w, double, double) {
            static_cast<LinuxGlfwBackend*>(glfwGetWindowUserPointer(w))->bump_interaction();
        });
        glfwSetMouseButtonCallback(window_, [](GLFWwindow* w, int, int, int) {
            static_cast<LinuxGlfwBackend*>(glfwGetWindowUserPointer(w))->bump_interaction();
        });
        glfwSetScrollCallback(window_, [](GLFWwindow* w, double, double) {
            static_cast<LinuxGlfwBackend*>(glfwGetWindowUserPointer(w))->bump_interaction();
        });
        glfwSetKeyCallback(window_, [](GLFWwindow* w, int, int, int, int) {
            static_cast<LinuxGlfwBackend*>(glfwGetWindowUserPointer(w))->bump_interaction();
        });

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        last_interaction_ = std::chrono::steady_clock::now();
        return true;
    }

    void shutdown() override {
        if (ImGui::GetCurrentContext()) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        }
        if (window_) {
            glfwDestroyWindow(window_);
            window_ = nullptr;
            glfwTerminate();
        }
    }

    bool should_close() const override {
        return !window_ || glfwWindowShouldClose(window_) != 0;
    }

    void wait_events(double timeout_sec) override {
        if (timeout_sec <= 0.0) {
            glfwPollEvents();
        } else {
            glfwWaitEventsTimeout(timeout_sec);
        }
    }

    bool begin_frame() override {
        if (should_close()) return false;
        if (glfwGetWindowAttrib(window_, GLFW_ICONIFIED)) {
            wait_events(0.1);
            return false;
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        return true;
    }

    void end_frame(float clear_r, float clear_g, float clear_b) override {
        ImGui::Render();
        int display_w = 0, display_h = 0;
        glfwGetFramebufferSize(window_, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_r, clear_g, clear_b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window_);
    }

    void set_dark_titlebar(bool dark) override { (void)dark; }
    float content_scale() const override { return scale_; }

    bool has_active_interaction() const override {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - last_interaction_).count();
        return elapsed < 450;
    }

    ImTextureID create_rgba_texture(int width, int height, const uint8_t* rgba) override {
        if (width <= 0 || height <= 0 || !rgba) return 0;
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
        glBindTexture(GL_TEXTURE_2D, 0);
        return static_cast<ImTextureID>(static_cast<uintptr_t>(tex));
    }

    void destroy_texture(ImTextureID tex) override {
        if (!tex) return;
        GLuint id = static_cast<GLuint>(static_cast<uintptr_t>(tex));
        glDeleteTextures(1, &id);
    }

private:
    void bump_interaction() {
        last_interaction_ = std::chrono::steady_clock::now();
    }

    GLFWwindow* window_ = nullptr;
    float scale_ = 1.0f;
    std::chrono::steady_clock::time_point last_interaction_{};
};
} // namespace

std::unique_ptr<GuiBackend> create_gui_backend() {
    return std::make_unique<LinuxGlfwBackend>();
}

} // namespace osmv::gui
