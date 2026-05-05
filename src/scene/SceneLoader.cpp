#include "scene/SceneLoader.hpp"

#include "scene/Scene.hpp"
#include "scene/Camera.hpp"
#include "scene/Material.hpp"
#include "scene/Light.hpp"
#include "geometry/ObjLoader.hpp"
#include "geometry/MeshFactory.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>

namespace astraglyph {
namespace {

using json = nlohmann::json;

[[nodiscard]] std::filesystem::path resolveAssetPathImpl(const std::filesystem::path& path)
{
  if (path.is_absolute()) {
    return path;
  }

  std::filesystem::path probe = std::filesystem::current_path();
  for (int depth = 0; depth < 8; ++depth) {
    const std::filesystem::path candidate = probe / path;
    if (std::filesystem::exists(candidate)) {
      return candidate;
    }
    if (!probe.has_parent_path()) {
      break;
    }
    const std::filesystem::path parent = probe.parent_path();
    if (parent == probe) {
      break;
    }
    probe = parent;
  }

  return path;
}

[[nodiscard]] Vec3 parseVec3(const json& value)
{
  if (!value.is_array() || value.size() < 3) {
    return Vec3{};
  }
  return Vec3{
      value[0].get<float>(),
      value[1].get<float>(),
      value[2].get<float>(),
  };
}

void parseRenderSettings(RenderSettings& settings, const json& obj)
{
  if (obj.contains("gridWidth")) {
    settings.gridWidth = obj["gridWidth"].get<int>();
  }
  if (obj.contains("gridHeight")) {
    settings.gridHeight = obj["gridHeight"].get<int>();
  }
  if (obj.contains("samplesPerCell")) {
    settings.samplesPerCell = obj["samplesPerCell"].get<int>();
  }
  if (obj.contains("maxSamplesPerCell")) {
    settings.maxSamplesPerCell = obj["maxSamplesPerCell"].get<int>();
  }
  if (obj.contains("jitteredSampling")) {
    settings.jitteredSampling = obj["jitteredSampling"].get<bool>();
  }
  if (obj.contains("adaptiveSampling")) {
    settings.adaptiveSampling = obj["adaptiveSampling"].get<bool>();
  }
  if (obj.contains("adaptiveVarianceThreshold")) {
    settings.adaptiveVarianceThreshold = obj["adaptiveVarianceThreshold"].get<float>();
  }
  if (obj.contains("temporalAccumulation")) {
    settings.temporalAccumulation = obj["temporalAccumulation"].get<bool>();
  }
  if (obj.contains("enableShadows")) {
    settings.enableShadows = obj["enableShadows"].get<bool>();
  }
  if (obj.contains("enableSoftShadows")) {
    settings.enableSoftShadows = obj["enableSoftShadows"].get<bool>();
  }
  if (obj.contains("shadowSamples")) {
    settings.shadowSamples = obj["shadowSamples"].get<int>();
  }
  if (obj.contains("enableReflections")) {
    settings.enableReflections = obj["enableReflections"].get<bool>();
  }
  if (obj.contains("maxBounces")) {
    settings.maxBounces = obj["maxBounces"].get<int>();
  }
  if (obj.contains("enableBvh")) {
    settings.enableBvh = obj["enableBvh"].get<bool>();
  }
  if (obj.contains("enableMultithreading")) {
    settings.enableMultithreading = obj["enableMultithreading"].get<bool>();
  }
  if (obj.contains("threadCount")) {
    settings.threadCount = obj["threadCount"].get<int>();
  }
  if (obj.contains("exposure")) {
    settings.exposure = obj["exposure"].get<float>();
  }
  if (obj.contains("gamma")) {
    settings.gamma = obj["gamma"].get<float>();
  }
  if (obj.contains("colorOutput")) {
    settings.colorOutput = obj["colorOutput"].get<bool>();
  }
  if (obj.contains("glyphRampMode")) {
    const std::string mode = obj["glyphRampMode"].get<std::string>();
    if (mode == "Filled") {
      settings.glyphRampMode = GlyphRampMode::Filled;
    } else {
      settings.glyphRampMode = GlyphRampMode::Classic;
    }
  }
  if (obj.contains("backfaceCulling")) {
    settings.backfaceCulling = obj["backfaceCulling"].get<bool>();
  }
  if (obj.contains("bvhLeafSize")) {
    settings.bvhLeafSize = obj["bvhLeafSize"].get<int>();
  }
  if (obj.contains("ambientStrength")) {
    settings.ambientStrength = obj["ambientStrength"].get<float>();
  }
  if (obj.contains("backgroundColor")) {
    settings.backgroundColor = parseVec3(obj["backgroundColor"]);
  }
  if (obj.contains("shadowBias")) {
    settings.shadowBias = obj["shadowBias"].get<float>();
  }
  if (obj.contains("reflectionBias")) {
    settings.reflectionBias = obj["reflectionBias"].get<float>();
  }
  if (obj.contains("debugAlbedoOnly")) {
    settings.debugAlbedoOnly = obj["debugAlbedoOnly"].get<bool>();
  }
  settings.validate();
}

void parseCamera(Camera& camera, const json& obj)
{
  if (obj.contains("position")) {
    const Vec3 pos = parseVec3(obj["position"]);
    camera.setPosition(pos);
  }
  if (obj.contains("yaw")) {
    camera.setYaw(obj["yaw"].get<float>());
  }
  if (obj.contains("pitch")) {
    camera.setPitch(obj["pitch"].get<float>());
  }
  if (obj.contains("fov")) {
    camera.setFovY(obj["fov"].get<float>());
  }
}

[[nodiscard]] std::unordered_map<std::string, std::size_t> parseMaterials(
    Scene& scene,
    const json& materialsArray)
{
  std::unordered_map<std::string, std::size_t> nameToIndex;
  if (!materialsArray.is_array()) {
    return nameToIndex;
  }

  for (std::size_t i = 0; i < materialsArray.size(); ++i) {
    const json& matObj = materialsArray[i];
    Material mat{};
    if (matObj.contains("albedo")) {
      mat.albedo = parseVec3(matObj["albedo"]);
    }
    if (matObj.contains("roughness")) {
      mat.roughness = matObj["roughness"].get<float>();
    }
    if (matObj.contains("metallic")) {
      mat.metallic = matObj["metallic"].get<float>();
    }
    if (matObj.contains("reflectivity")) {
      mat.reflectivity = matObj["reflectivity"].get<float>();
    }
    if (matObj.contains("emissionColor")) {
      mat.emissionColor = parseVec3(matObj["emissionColor"]);
    }
    if (matObj.contains("emissionStrength")) {
      mat.emissionStrength = matObj["emissionStrength"].get<float>();
    }
    
    // Support loading materials from MTL file
    if (matObj.contains("mtlPath")) {
      const std::filesystem::path mtlPath = resolveAssetPathImpl(matObj["mtlPath"].get<std::string>());
      const std::vector<Material> mtlMaterials = ObjLoader::loadMaterials(mtlPath, true);
      if (!mtlMaterials.empty()) {
        // Use the first material from MTL file
        mat = mtlMaterials[0];
      }
    }
    
    scene.addMaterial(mat);

    if (matObj.contains("name")) {
      nameToIndex[matObj["name"].get<std::string>()] = i;
    }
  }

  return nameToIndex;
}

void parseLights(Scene& scene, const json& lightsArray)
{
  if (!lightsArray.is_array()) {
    return;
  }

  for (const json& lightObj : lightsArray) {
    if (!lightObj.is_object()) {
      continue;
    }

    Light light{};
    if (lightObj.contains("type")) {
      const std::string typeStr = lightObj["type"].get<std::string>();
      if (typeStr == "point") {
        light.type = LightType::Point;
      } else if (typeStr == "area") {
        light.type = LightType::Area;
      } else {
        light.type = LightType::Directional;
      }
    }
    if (lightObj.contains("direction")) {
      light.direction = parseVec3(lightObj["direction"]);
    }
    if (lightObj.contains("position")) {
      light.position = parseVec3(lightObj["position"]);
    }
    if (lightObj.contains("color")) {
      light.color = parseVec3(lightObj["color"]);
    }
    if (lightObj.contains("intensity")) {
      light.intensity = lightObj["intensity"].get<float>();
    }
    if (lightObj.contains("right")) {
      light.right = parseVec3(lightObj["right"]);
    }
    if (lightObj.contains("up")) {
      light.up = parseVec3(lightObj["up"]);
    }
    if (lightObj.contains("width")) {
      light.width = lightObj["width"].get<float>();
    }
    if (lightObj.contains("height")) {
      light.height = lightObj["height"].get<float>();
    }
    scene.addLight(light);
  }
}

void parseObjects(
    Scene& scene,
    const json& objectsArray,
    const std::unordered_map<std::string, std::size_t>& materialMap)
{
  if (!objectsArray.is_array()) {
    return;
  }

  for (const json& obj : objectsArray) {
    if (!obj.is_object()) {
      continue;
    }

    std::size_t materialIndex = 0;
    if (obj.contains("material")) {
      const std::string matName = obj["material"].get<std::string>();
      const auto it = materialMap.find(matName);
      if (it != materialMap.end()) {
        materialIndex = it->second;
      }
    }

    Mesh mesh{static_cast<int>(materialIndex)};
    const std::string type = obj.contains("type") ? obj["type"].get<std::string>() : "cube";

    if (type == "plane") {
      float width = 5.0F;
      float depth = 5.0F;
      if (obj.contains("size")) {
        const Vec3 size = parseVec3(obj["size"]);
        width = size.x;
        depth = size.z;
      }
      mesh = MeshFactory::createPlane(width, depth, static_cast<int>(materialIndex));
    } else if (type == "cube") {
      float size = 1.0F;
      if (obj.contains("size")) {
        const Vec3 s = parseVec3(obj["size"]);
        mesh = MeshFactory::createCube(s, static_cast<int>(materialIndex));
      } else {
        mesh = MeshFactory::createCube(size, static_cast<int>(materialIndex));
      }
    } else if (type == "sphere") {
      float radius = 0.5F;
      int segments = 24;
      int rings = 12;
      if (obj.contains("radius")) {
        radius = obj["radius"].get<float>();
      }
      if (obj.contains("segments")) {
        segments = obj["segments"].get<int>();
      }
      if (obj.contains("rings")) {
        rings = obj["rings"].get<int>();
      }
      mesh = MeshFactory::createUvSphere(radius, segments, rings, static_cast<int>(materialIndex));
    } else if (type == "obj") {
      if (obj.contains("path")) {
        const std::filesystem::path objPath = resolveAssetPathImpl(obj["path"].get<std::string>());
        try {
          // Load materials from MTL file FIRST, before loading the mesh
          std::filesystem::path mtlPath;
          if (obj.contains("mtlPath")) {
            mtlPath = resolveAssetPathImpl(obj["mtlPath"].get<std::string>());
          } else {
            // Try to find MTL file next to OBJ
            mtlPath = objPath.parent_path() / (objPath.stem().string() + ".mtl");
          }
          
          // Load MTL materials and add to scene before loading mesh
          std::size_t mtlMaterialStartIndex = scene.getMaterialCount();
          if (std::filesystem::exists(mtlPath)) {
            const std::vector<Material> mtlMaterials = ObjLoader::loadMaterials(mtlPath, true);
            for (const auto& mtlMat : mtlMaterials) {
              scene.addMaterial(mtlMat);
            }
          }
          
          // If no MTL material loaded, add a default white material
          if (scene.getMaterialCount() == mtlMaterialStartIndex) {
            scene.addMaterial(Material{});  // Default: white albedo (1,1,1)
          }
          
          // Use the first MTL material index for all triangles in this OBJ
          const std::size_t objMaterialIndex = mtlMaterialStartIndex;
          mesh = ObjLoader::loadFromFile(objPath, ObjLoadOptions{static_cast<int>(objMaterialIndex)});
        } catch (const std::exception&) {
          mesh = MeshFactory::createCube(0.1F, static_cast<int>(materialIndex));
        }
      }
    }

    if (obj.contains("transform")) {
      const json& transformObj = obj["transform"];
      if (transformObj.contains("scale")) {
        mesh.scale(parseVec3(transformObj["scale"]));
      }
      if (transformObj.contains("rotation")) {
        const Vec3 rot = parseVec3(transformObj["rotation"]);
        mesh.rotate(rot.x, rot.y, rot.z);
      }
      if (transformObj.contains("position")) {
        mesh.translate(parseVec3(transformObj["position"]));
      }
    }

    scene.addMesh(std::move(mesh));
  }
}

} // namespace

std::filesystem::path SceneLoader::resolveAssetPath(const std::filesystem::path& path)
{
  return resolveAssetPathImpl(path);
}

SceneLoadResult SceneLoader::loadFromFile(const std::filesystem::path& path)
{
  const std::filesystem::path resolved = resolveAssetPath(path);

  if (!std::filesystem::exists(resolved)) {
    SceneLoadResult result = loadDefaultScene();
    result.errorMessage = "Scene file not found: " + path.string() + ", using fallback";
    return result;
  }

  try {
    std::ifstream file(resolved);
    if (!file.is_open()) {
      SceneLoadResult result = loadDefaultScene();
      result.errorMessage = "Cannot open scene file: " + path.string() + ", using fallback";
      return result;
    }

    json root;
    file >> root;

    SceneLoadResult result;
    result.success = true;

    if (root.contains("camera")) {
      parseCamera(result.camera, root["camera"]);
    }
    if (root.contains("renderSettings")) {
      parseRenderSettings(result.renderSettings, root["renderSettings"]);
    }

    std::unordered_map<std::string, std::size_t> materialMap;
    if (root.contains("materials")) {
      materialMap = parseMaterials(result.scene, root["materials"]);
    }

    if (root.contains("lights")) {
      parseLights(result.scene, root["lights"]);
    }

    if (root.contains("objects")) {
      parseObjects(result.scene, root["objects"], materialMap);
    }

    return result;
  } catch (const std::exception& e) {
    SceneLoadResult result = loadDefaultScene();
    result.errorMessage = std::string("Failed to parse scene: ") + e.what() + ", using fallback";
    return result;
  }
}

SceneLoadResult SceneLoader::loadDefaultScene()
{
  SceneLoadResult result;
  result.scene = Scene::createDefaultScene();
  result.camera.setPosition(Vec3{0.0F, 0.0F, 0.0F});
  result.camera.setYaw(0.0F);
  result.camera.setPitch(0.0F);
  result.camera.setFovY(70.0F);
  result.renderSettings = RenderSettings{};
  result.renderSettings.validate();
  result.success = true;
  return result;
}

} // namespace astraglyph
