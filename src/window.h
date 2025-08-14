//
// Created by Tonz on 16.07.2025.
//
#pragma once

#include <cstdint>
#include <string>

#include "observers.h"
#include "GLFW/glfw3.h"

#include <glm/glm.hpp>

class Window : public KeyPressObserver {
public:
    Window(std::string_view name, uint32_t width, uint32_t height, bool fullscreen);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;


    [[nodiscard]] GLFWwindow *getGlfwWindow() const {
        return glfwWindow_;
    }

    void update(int keyCode) override;

    enum class CursorMode {
        normal = GLFW_CURSOR_NORMAL,
        disabled = GLFW_CURSOR_DISABLED,
    };

    enum class WindowMode {
        windowed,
        fullscreen
    };

private:

    void init();

    uint32_t width_{};
    uint32_t height_{};
    bool fullscreen_{};

    GLFWwindow* glfwWindow_{nullptr};
    std::string name_{};
    CursorMode cursorMode_{CursorMode::disabled};
    WindowMode windowMode_{WindowMode::windowed};

    glm::ivec2 lastWindowPos_{};
    glm::ivec2 lastWindowSize_{};
};