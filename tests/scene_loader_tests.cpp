#include "scene/SceneLoader.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace {

void writeTempScene(const std::filesystem::path& path)
{
  std::ofstream file(path);
  file << R"({
  "camera": {
    "position": [1.0, 2.0, 3.0],
    "yaw": 45.0,
    "pitch": -15.0,
    "fov": 60.0
  },
  "renderSettings": {
    "gridWidth": 80,
    "gridHeight": 45,
    "samplesPerCell": 2,
    "enableShadows": false
  },
  "materials": [
    {"name": "red", "albedo": [1.0, 0.0, 0.0], "roughness": 0.5},
    {"name": "green", "albedo": [0.0, 1.0, 0.0], "roughness": 0.5}
  ],
  "lights": [
    {"type": "directional", "direction": [0.0, -1.0, 0.0], "color": [1.0, 1.0, 1.0], "intensity": 1.0},
    {"type": "point", "position": [0.0, 1.0, 0.0], "color": [1.0, 0.0, 0.0], "intensity": 0.5}
  ],
  "objects": [
    {"type": "plane", "material": "red", "size": [10.0, 1.0, 10.0], "transform": {"position": [0.0, -1.0, 0.0]}},
    {"type": "cube", "material": "green", "size": [1.0, 1.0, 1.0], "transform": {"position": [1.0, 0.0, 0.0]}},
    {"type": "sphere", "material": "red", "radius": 0.5, "segments": 12, "rings": 6, "transform": {"position": [-1.0, 0.0, 0.0]}}
  ]
})";
}

} // namespace

int main()
{
  // Test 1: Missing file fallback
  {
    const auto result = astraglyph::SceneLoader::loadFromFile("nonexistent_scene_file.json");
    assert(result.success);
    assert(!result.scene.meshes().empty());
    assert(!result.errorMessage.empty());
  }

  // Test 2: Load valid scene
  {
    const std::filesystem::path tempPath = "temp_test_scene.json";
    writeTempScene(tempPath);

    const auto result = astraglyph::SceneLoader::loadFromFile(tempPath);
    assert(result.success);
    assert(!result.scene.meshes().empty());
    assert(!result.scene.lights().empty());
    assert(!result.scene.materials().empty());

    // Test camera parsing
    const astraglyph::Vec3 pos = result.camera.position();
    assert(pos.x == 1.0F);
    assert(pos.y == 2.0F);
    assert(pos.z == 3.0F);

    // Test renderSettings parsing
    assert(result.renderSettings.gridWidth == 80);
    assert(result.renderSettings.gridHeight == 45);
    assert(result.renderSettings.samplesPerCell == 2);
    assert(result.renderSettings.enableShadows == false);

    // Test materials loading
    assert(result.scene.materials().size() == 2);
    const auto& redMat = result.scene.materials()[0];
    assert(redMat.albedo.x == 1.0F);
    assert(redMat.albedo.y == 0.0F);
    assert(redMat.albedo.z == 0.0F);

    // Test lights loading
    assert(result.scene.lights().size() == 2);
    assert(result.scene.lights()[0].type == astraglyph::LightType::Directional);
    assert(result.scene.lights()[1].type == astraglyph::LightType::Point);

    // Test objects loading
    assert(result.scene.meshes().size() == 3);

    std::filesystem::remove(tempPath);
  }

  // Test 3: Default scene fallback
  {
    const auto result = astraglyph::SceneLoader::loadDefaultScene();
    assert(result.success);
    assert(!result.scene.meshes().empty());
    assert(!result.scene.lights().empty());
  }

  std::printf("scene_loader_tests: all passed\n");
  return 0;
}
