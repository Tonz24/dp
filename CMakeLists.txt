cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_STANDARD 23)
project(dp)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/libs/imgui)

add_executable(dp
        src/entry/main.cpp
        src/engine.cpp
        src/engine.h
        src/window.cpp
        src/window.h
        src/utils.cpp
        src/utils.h
        src/observer.h
        src/engine/transform.cpp
        src/engine/transform.h
        src/engine/texture.cpp
        src/engine/texture.h
        src/engine/material.cpp
        src/engine/material.h
        src/engine/scene/modelLoader.cpp
        src/engine/scene/modelLoader.h
        src/engine/managers/resourceManager.cpp
        src/engine/managers/resourceManager.h
        src/engine/managers/managedResource.cpp
        src/engine/managers/managedResource.h
        src/engine/managers/textureManager.cpp
        src/engine/managers/textureManager.h
        src/engine/managers/materialManager.cpp
        src/engine/managers/materialManager.h
        src/engine/scene/vertex.h
        src/engine/scene/mesh.cpp
        src/engine/scene/mesh.h
        src/engine/managers/resourceManagerBase.cpp
        src/engine/managers/resourceManagerBase.h
        src/engine/managers/meshManager.cpp
        src/engine/managers/meshManager.h
        src/engine/scene/camera.cpp
        src/engine/scene/camera.h
        src/engine/scene/scene.cpp
        src/engine/scene/scene.h
        src/engine/scene/model.cpp
        src/engine/scene/model.h

        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_impl_vulkan.cpp
        ${IMGUI_DIR}/imgui_impl_glfw.cpp
        src/engine/managers/inputManager.cpp
        src/engine/managers/inputManager.h
        src/observers.h
        src/uboFormat.h
)

# add shader compilation as a build step
include(cmake/shader_compilation.cmake)
add_dependencies(dp SHADER_COMPILATION)

# packages
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(freeimage CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(OpenMP REQUIRED)
# find_package(imgui CONFIG REQUIRED)


target_compile_definitions(${PROJECT_NAME}
        PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/libs)


target_link_libraries(${PROJECT_NAME} PRIVATE
        Vulkan::Vulkan
        glfw
        freeimage::FreeImage
        freeimage::FreeImagePlus
        assimp::assimp
        OpenMP::OpenMP_CXX
)

# keep this here otherwise MinGW breaks FreeImage on linking as 'ws2_32 gdi32 user32 shell32' will be in the middle of files to link
if(MINGW)
        target_link_libraries(freeimage::FreeImage INTERFACE
                ws2_32 gdi32 user32 shell32
        )
endif()