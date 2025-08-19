#include "../engine/engine.h"
#include "../engine/modelLoader.h"


int main() {
    Engine::getInstance().init();

    auto model = ModelLoader::loadModel("room/room.obj");

    auto cam = std::make_shared<Camera>(glm::vec3{0,0,2},glm::vec3{0,0,0});

    auto scene = std::make_shared<Scene>(model,cam);

    Engine::getInstance().setScene(scene);
    Engine::getInstance().run();
    return 0;
}
