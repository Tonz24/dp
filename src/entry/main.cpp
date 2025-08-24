#include "../engine/engine.h"
#include "../engine/modelLoader.h"
#include "vma/vk_mem_alloc.h"

int main() {
    Engine::getInstance().init();

    auto cam = std::make_shared<Camera>(glm::vec3{0,0,2},glm::vec3{0,0,0});

    auto scene = std::make_shared<Scene>(ModelLoader::loadModel("room/room.obj", false),cam);

    Engine::getInstance().setScene(std::move(scene));
    Engine::getInstance().run();
    Engine::getInstance().cleanup();

    return 0;
}
