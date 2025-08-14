//
// Created by Tonz on 11.08.2025.
//

#pragma once

#include <iostream>
#include <GLFW/glfw3.h>

#include "../../observers.h"

class InputManager : public KeyPressSubject {
public:
    static InputManager& getInstance(){
        if(instance == nullptr)
            instance = new InputManager();
        return *instance;
    }

    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods){
        getInstance().keyCallbackInternal(window,key,scancode,action,mods);
    }

private:
    inline static InputManager* instance{nullptr};

    void keyCallbackInternal(GLFWwindow *window, int key, int scancode, int action, int mods){
        if (action == GLFW_PRESS) {
            notify(key);
        }
    }
};
