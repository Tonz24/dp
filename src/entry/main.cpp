

#include "../engine/engine.h"
#include "../engine/modelLoader.h"
#include "../engine/managers/resourceManager.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/string_cast.hpp>

glm::vec2 map(glm::vec3 dir) {
    float u = 0.5f + 0.5f * glm::atan(dir.z, dir.x) * glm::one_over_pi<float>();
    float v = 1.0f - glm::acos(dir.y) * glm::one_over_pi<float>();
    return {u,v};
}

void testMapping() {
    std::vector v = {
        glm::vec3{0,1,0},
        glm::vec3{0,-1,0},
        glm::vec3{0,0,1},
        glm::vec3{0,0,-1},
        glm::vec3{1,0,0},
        glm::vec3{-1,0,0},
    };

    for (const auto & value : v) {
        std::cout << glm::to_string(map(value)) << std::endl;
    } 
}


int main() {

    //testMapping();
    //return 0;c
    Engine::getInstance().init();

    auto cam = std::make_shared<Camera>(glm::vec3{0,0,2},glm::vec3{0,0,0});
    //auto sky = TextureManager::getInstance()->registerResource("sky", "../assets/sky/lebombo_4k.exr", false, false);
    //auto sky = TextureManager::getInstance()->registerResource("sky", "../assets/sky/satara_night_1k.exr", false, false);
    auto sky = TextureManager::getInstance()->registerResource("sky", "../assets/sky/test.jpg", true, false);
    //auto scene = std::make_shared<Scene>(ModelLoader::loadModel("room/room.obj", false),cam,std::move(sky));
    //auto scene = std::make_shared<Scene>(ModelLoader::loadModel("living_room/living_room.obj", false),cam,std::move(sky));
    auto scene = std::make_shared<Scene>(ModelLoader::loadModel("cornell/cornell_normal.obj", false),cam,std::move(sky));

    Engine::getInstance().setScene(std::move(scene));
    Engine::getInstance().run();
    Engine::getInstance().cleanup();

    return EXIT_SUCCESS;
}