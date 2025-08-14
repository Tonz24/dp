//
// Created by Tonz on 16.07.2025.
//

#include "window.h"

#include <iostream>

#include "engine/managers/inputManager.h"

Window::Window(std::string_view name, uint32_t width, uint32_t height, bool fullscreen) : width_(width), height_(height), fullscreen_(fullscreen), name_(name){
    init();
    glfwGetWindowPos(glfwWindow_,&lastWindowPos_.x,&lastWindowPos_.y);

    InputManager::getInstance().attach(this);
}

Window::~Window() {
    InputManager::getInstance().detach(this);
    glfwDestroyWindow(glfwWindow_);
}

void Window::update(int keyCode) {
    if (keyCode == GLFW_KEY_C) {

        if (cursorMode_ == CursorMode::disabled)
            cursorMode_ = CursorMode::normal;
        else if (cursorMode_ == CursorMode::normal)
            cursorMode_ = CursorMode::disabled;
        glfwSetInputMode(glfwWindow_, GLFW_CURSOR, static_cast<int>(cursorMode_));
    }
    if (keyCode == GLFW_KEY_F) {
        //  transition to fullscreen
        if (windowMode_ == WindowMode::windowed) {
            fullscreen_ = true;
            windowMode_ = WindowMode::fullscreen;

            glfwGetWindowPos(glfwWindow_,&lastWindowPos_.x,&lastWindowPos_.y);
            glfwGetWindowSize(glfwWindow_,&lastWindowSize_.x,&lastWindowSize_.y);

            glfwSetWindowMonitor(glfwWindow_,glfwGetPrimaryMonitor(),lastWindowPos_.x,lastWindowPos_.y,lastWindowSize_.x,lastWindowSize_.y,GLFW_DONT_CARE);
            fullscreen_ = true;
        }
        //  transition to windowed
        else if (windowMode_ == WindowMode::fullscreen) {
            windowMode_ = WindowMode::windowed;
            glfwSetWindowMonitor(glfwWindow_, nullptr,lastWindowPos_.x,lastWindowPos_.y,lastWindowSize_.x,lastWindowSize_.y,GLFW_DONT_CARE);
            fullscreen_ = false;
        }
    }
    if (keyCode == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(glfwWindow_, GLFW_TRUE);
}

void Window::init() {

    GLFWmonitor * initInFullscreen = fullscreen_ ? glfwGetPrimaryMonitor() : nullptr;
    glfwWindow_ = glfwCreateWindow(static_cast<int>(width_), static_cast<int>(height_), name_.c_str(),  initInFullscreen, nullptr);

    if (glfwWindow_ == nullptr) {
        std::cerr << "ERROR: Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

}


