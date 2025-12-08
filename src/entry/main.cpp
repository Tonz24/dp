

#include "../engine/engine.h"
#include "../engine/modelLoader.h"
#include "../engine/managers/resourceManager.h"

int main() {
    Engine::getInstance().init();

    auto cam = std::make_shared<Camera>(glm::vec3{0,0,5},glm::vec3{0,0,0});
    //auto sky = TextureManager::getInstance()->registerResource("sky", "../assets/envmaps/test.jpg", true, false);
    auto sky = TextureManager::getInstance()->registerResource("sky", "../assets/envmaps/gray_4k.exr", true, false);

    //auto scene = std::make_shared<Scene>(ModelLoader::loadModel("sphere/sphere.obj", false),cam,std::move(sky));
    //auto scene = std::make_shared<Scene>(ModelLoader::loadModel("fireplace/fireplace_room.obj", false),cam,std::move(sky));
    //auto scene = std::make_shared<Scene>(ModelLoader::loadModel("living_room/living_room2.obj", false),cam,std::move(sky));
    auto scene = std::make_shared<Scene>(ModelLoader::loadModel("cornell/cornell.obj", false),cam,std::move(sky));
    //auto scene = std::make_shared<Scene>(ModelLoader::loadModel("car/car.obj", false),cam,std::move(sky));

    //auto scene = std::make_shared<Scene>(ModelLoader::loadModel("cornell/cornell.obj", false),cam,std::move(sky));

    Engine::getInstance().setScene(std::move(scene));
    Engine::getInstance().run();
    Engine::getInstance().cleanup();

    return EXIT_SUCCESS;
}