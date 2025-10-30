

#include "../engine/engine.h"
#include "../engine/modelLoader.h"
#include "../engine/managers/resourceManager.h"

int main() {
    Engine::getInstance().init();

    auto cam = std::make_shared<Camera>(glm::vec3{0,0,2},glm::vec3{0,0,0});
    //auto sky = TextureManager::getInstance()->registerResource("sky", "../assets/sky/blaubeuren_night_1k.exr", false, false);
    //auto sky = TextureManager::getInstance()->registerResource("sky", "../assets/sky/satara_night_1k.exr", false, false);
    //auto sky = TextureManager::getInstance()->registerResource("sky", "../assets/sky/poolbeg_1k.exr", false, false);
    //auto sky = TextureManager::getInstance()->registerResource("sky", "../assets/sky/gray_4k.exr", false, false);
    //auto sky = TextureManager::getInstance()->registerResource("sky", "../assets/sky/asphalt.exr", false, false);
    //auto sky = TextureManager::getInstance()->registerResource("sky", "../assets/sky/studio2_4k.exr", false, false);
    //auto sky = TextureManager::getInstance()->registerResource("sky", "../assets/sky/studio_4k.exr", false, false);
    //auto sky = TextureManager::getInstance()->registerResource("sky", "../assets/sky/lebombo_4k.exr", false, false);
    //auto sky = TextureManager::getInstance()->registerResource("sky", "../assets/sky/test.jpg", true, false);
    auto sky = TextureManager::getInstance()->registerResource("sky", "../assets/sky/black.png", true, false);

    //auto scene = std::make_shared<Scene>(ModelLoader::loadModel("room/room2.obj", false),cam,std::move(sky));
    //auto scene = std::make_shared<Scene>(ModelLoader::loadModel("living_room/living_room2.obj", false),cam,std::move(sky));
    //auto scene = std::make_shared<Scene>(ModelLoader::loadModel("car/car.obj", false),cam,std::move(sky));

    auto scene = std::make_shared<Scene>(ModelLoader::loadModel("cornell/cornell_normal.obj", false),cam,std::move(sky));

    Engine::getInstance().setScene(std::move(scene));
    Engine::getInstance().run();
    Engine::getInstance().cleanup();

    return EXIT_SUCCESS;
}