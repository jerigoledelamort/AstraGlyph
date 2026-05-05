#include "geometry/ObjLoader.hpp"
#include "scene/Material.hpp"
#include "scene/SceneLoader.hpp"
#include <iostream>

int main() {
    // Test loading scene
    std::cout << "Loading scene from: assets/scenes/samurai_girl_scene.json" << std::endl;
    
    const auto result = astraglyph::SceneLoader::loadFromFile("assets/scenes/samurai_girl_scene.json");
    
    std::cout << "Scene loaded: " << (result.success ? "YES" : "NO") << std::endl;
    std::cout << "Materials count: " << result.scene.materials().size() << std::endl;
    std::cout << "Meshes count: " << result.scene.meshes().size() << std::endl;
    std::cout << "Triangles count: " << result.scene.triangles().size() << std::endl;
    
    for (size_t i = 0; i < result.scene.materials().size(); ++i) {
        const auto& mat = result.scene.materials()[i];
        std::cout << "\nMaterial " << i << ":" << std::endl;
        std::cout << "  albedo: " << mat.albedo.x << ", " << mat.albedo.y << ", " << mat.albedo.z << std::endl;
        std::cout << "  metallic: " << mat.metallic << std::endl;
        std::cout << "  roughness: " << mat.roughness << std::endl;

        std::cout << "  hasAlbedoTexture: " << (mat.hasAlbedoTexture ? "YES" : "NO") << std::endl;
        if (mat.hasAlbedoTexture) {
            std::cout << "    size: " << mat.albedoTextureWidth << "x" << mat.albedoTextureHeight
                      << ", channels: " << mat.albedoTextureChannels << std::endl;
        }

        std::cout << "  hasMetallicTexture: " << (mat.hasMetallicTexture ? "YES" : "NO") << std::endl;
        if (mat.hasMetallicTexture) {
            std::cout << "    size: " << mat.metallicTextureWidth << "x" << mat.metallicTextureHeight
                      << ", channels: " << mat.metallicTextureChannels << std::endl;
        }

        std::cout << "  hasRoughnessTexture: " << (mat.hasRoughnessTexture ? "YES" : "NO") << std::endl;
        if (mat.hasRoughnessTexture) {
            std::cout << "    size: " << mat.roughnessTextureWidth << "x" << mat.roughnessTextureHeight
                      << ", channels: " << mat.roughnessTextureChannels << std::endl;
        }
    }
    
    if (!result.scene.meshes().empty()) {
        const auto& mesh = result.scene.meshes()[0];
        std::cout << "\nMesh materialId: " << mesh.materialId() << std::endl;
    }
    
    if (!result.scene.triangles().empty()) {
        const auto& tri = result.scene.triangles()[0];
        std::cout << "First triangle materialId: " << tri.materialId << std::endl;
    }
    
    return 0;
}
