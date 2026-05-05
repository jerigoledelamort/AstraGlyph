#include "render/Renderer.hpp"
#include "render/AsciiMapper.hpp"
#include "scene/Scene.hpp"
#include "scene/Material.hpp"
#include "scene/Light.hpp"
#include "scene/Camera.hpp"
#include "geometry/MeshFactory.hpp"

#include <iostream>
#include <cassert>

using namespace astraglyph;

int main() {
  std::cout << "=== Render Color Tests ===" << std::endl;
  
  // Test 1: Pure white material with no texture
  std::cout << "\nTest 1: Pure white material (albedo 1,1,1)" << std::endl;
  {
    Scene scene;
    Material mat{};
    mat.albedo = Vec3{1.0F, 1.0F, 1.0F};
    mat.roughness = 1.0F;
    mat.metallic = 0.0F;
    mat.reflectivity = 0.0F;
    scene.addMaterial(mat);
    scene.addLight(Light{LightType::Directional, Vec3{0.0F, 0.0F, -1.0F}, Vec3{}, Vec3{1.0F, 1.0F, 1.0F}, 1.0F});
    
    Mesh cube = MeshFactory::createCube(1.0F, 0);
    scene.addMesh(std::move(cube));
    
    Camera camera;
    camera.setPosition(Vec3{0.0F, 0.0F, 3.0F});
    camera.setPitch(0.0F);
    camera.setYaw(0.0F);
    camera.setAspect(4.0F / 3.0F);
    camera.setFovY(60.0F);
    
    RenderSettings settings;
    settings.gridWidth = 10;
    settings.gridHeight = 10;
    settings.samplesPerCell = 1;
    settings.exposure = 1.0F;
    settings.gamma = 1.0F;
    settings.colorOutput = false;
    settings.enableShadows = false;
    settings.ambientStrength = 1.0F;  // Full ambient
    
    Renderer renderer;
    AsciiFramebuffer framebuffer(10, 10);
    renderer.render(framebuffer, camera, scene, settings);
    
    // Check center cell
    const AsciiCell& centerCell = framebuffer.at(5, 5);
    std::cout << "  Center cell fg: (" 
              << centerCell.fg.x << ", " 
              << centerCell.fg.y << ", " 
              << centerCell.fg.z << ")" << std::endl;
    std::cout << "  Center cell luminance: " << centerCell.luminance << std::endl;
    
    // With ambientStrength=1.0 and albedo=(1,1,1), we expect:
    // ambient = 1.0 * 1.0 = 1.0 (before tone mapping)
    // After tone mapping: 1 - exp(-1.0) = 0.632
    // With gamma=1.0: 0.632
    float expected = 1.0F - std::exp(-1.0F);
    std::cout << "  Expected (ambient=1.0, tone mapped): " << expected << std::endl;
    
    if (std::fabs(centerCell.fg.x - expected) < 0.05F &&
        std::fabs(centerCell.fg.y - expected) < 0.05F &&
        std::fabs(centerCell.fg.z - expected) < 0.05F) {
      std::cout << "  ✓ PASS: Ambient lighting is correct" << std::endl;
    } else {
      std::cout << "  ✗ FAIL: Ambient is wrong! Difference: " 
                << std::fabs(centerCell.fg.x - expected) << std::endl;
    }
    
    // Check for color imbalance (R > G ≈ B indicates red tint)
    float rDiff = centerCell.fg.x - centerCell.fg.y;
    float bDiff = centerCell.fg.z - centerCell.fg.y;
    if (std::fabs(rDiff) > 0.1F || std::fabs(bDiff) > 0.1F) {
      std::cout << "  ✗ FAIL: Color imbalance detected! R-G=" << rDiff << ", B-G=" << bDiff << std::endl;
    } else {
      std::cout << "  ✓ PASS: Color is balanced" << std::endl;
    }
  }
    
  // Test 2: Check tone mapping with gamma=1.0
  std::cout << "\nTest 2: Tone mapping with gamma=1.0 (linear)" << std::endl;
  {
    AsciiMapper mapper;
    
    // Pure white input
    Vec3 white = {1.0F, 1.0F, 1.0F};
    Vec3 result = mapper.applyExposureGamma(white, 1.0F, 1.0F);
    
    std::cout << "  Input: (1,1,1), exposure=1.0, gamma=1.0" << std::endl;
    std::cout << "  Output: (" << result.x << ", " << result.y << ", " << result.z << ")" << std::endl;
    
    // With tone mapping: 1 - exp(-1.0) = 0.632
    // With gamma=1.0: pow(0.632, 1.0) = 0.632
    float expected = 1.0F - std::exp(-1.0F);
    std::cout << "  Expected (approx): " << expected << std::endl;
    
    if (std::fabs(result.x - expected) < 0.01F &&
        std::fabs(result.y - expected) < 0.01F &&
        std::fabs(result.z - expected) < 0.01F) {
      std::cout << "  ✓ PASS: Tone mapping is correct" << std::endl;
    } else {
      std::cout << "  ✗ FAIL: Tone mapping is incorrect" << std::endl;
    }
  }
    
  // Test 3: Check for RGB channel order issue
  std::cout << "\nTest 3: RGB channel order check" << std::endl;
  {
    AsciiMapper mapper;
    
    // Red-only input
    Vec3 red = {1.0F, 0.0F, 0.0F};
    Vec3 redResult = mapper.applyExposureGamma(red, 1.0F, 1.0F);
    
    // Green-only input
    Vec3 green = {0.0F, 1.0F, 0.0F};
    Vec3 greenResult = mapper.applyExposureGamma(green, 1.0F, 1.0F);
    
    // Blue-only input
    Vec3 blue = {0.0F, 0.0F, 1.0F};
    Vec3 blueResult = mapper.applyExposureGamma(blue, 1.0F, 1.0F);
    
    std::cout << "  Red input (1,0,0) -> (" << redResult.x << ", " << redResult.y << ", " << redResult.z << ")" << std::endl;
    std::cout << "  Green input (0,1,0) -> (" << greenResult.x << ", " << greenResult.y << ", " << greenResult.z << ")" << std::endl;
    std::cout << "  Blue input (0,0,1) -> (" << blueResult.x << ", " << blueResult.y << ", " << blueResult.z << ")" << std::endl;
    
    // Check that red stays in X channel
    if (redResult.x > 0.5F && redResult.y < 0.1F && redResult.z < 0.1F) {
      std::cout << "  ✓ PASS: Red channel is correct" << std::endl;
    } else {
      std::cout << "  ✗ FAIL: Red channel is wrong!" << std::endl;
    }
    
    // Check that green stays in Y channel
    if (greenResult.x < 0.1F && greenResult.y > 0.5F && greenResult.z < 0.1F) {
      std::cout << "  ✓ PASS: Green channel is correct" << std::endl;
    } else {
      std::cout << "  ✗ FAIL: Green channel is wrong!" << std::endl;
    }
    
    // Check that blue stays in Z channel
    if (blueResult.x < 0.1F && blueResult.y < 0.1F && blueResult.z > 0.5F) {
      std::cout << "  ✓ PASS: Blue channel is correct" << std::endl;
    } else {
      std::cout << "  ✗ FAIL: Blue channel is wrong!" << std::endl;
    }
  }
  
  std::cout << "\n=== Tests complete ===" << std::endl;
  return 0;
}
