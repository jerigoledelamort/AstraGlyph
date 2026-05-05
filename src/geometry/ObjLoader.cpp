#include "geometry/ObjLoader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "D:/Projects/AstraGlyph/third_party/stb/stb_image.h"

#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace astraglyph {
namespace {

constexpr float kNormalEpsilon = 1.0e-6F;

struct FaceVertex {
  std::size_t positionIndex{0};
  std::size_t texCoordIndex{0};
  std::size_t normalIndex{0};
  bool hasTexCoord{false};
  bool hasNormal{false};
};

struct MtlMaterial {
  std::string name;
  Vec3 albedo{1.0F, 1.0F, 1.0F};
  float roughness{0.5F};
  float metallic{0.0F};
  float reflectivity{0.0F};
  Vec3 emissionColor{0.0F, 0.0F, 0.0F};
  float emissionStrength{0.0F};
  std::string albedoTexturePath;
  std::string metallicTexturePath;
  std::string roughnessTexturePath;
};

[[nodiscard]] std::string trim(std::string_view text)
{
  std::size_t begin = 0;
  while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
    ++begin;
  }

  std::size_t end = text.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
    --end;
  }

  return std::string{text.substr(begin, end - begin)};
}

[[nodiscard]] std::string buildContextLabel(const std::filesystem::path& sourcePath)
{
  return sourcePath.empty() ? std::string{"OBJ"} : sourcePath.string();
}

[[noreturn]] void fail(const std::filesystem::path& sourcePath, std::size_t lineNumber, std::string_view message)
{
  throw std::runtime_error(
      buildContextLabel(sourcePath) + ":" + std::to_string(lineNumber) + ": " + std::string{message});
}

[[nodiscard]] int parseSignedIndex(
    std::string_view text,
    const std::filesystem::path& sourcePath,
    std::size_t lineNumber,
    std::string_view label)
{
  if (text.empty()) {
    fail(sourcePath, lineNumber, "empty OBJ index in face element");
  }

  std::size_t offset = 0;
  if (text.front() == '-') {
    offset = 1;
  }

  if (offset == text.size()) {
    fail(sourcePath, lineNumber, std::string{"invalid OBJ index '"} + std::string{text} + "'");
  }

  int value = 0;
  for (std::size_t index = offset; index < text.size(); ++index) {
    const unsigned char ch = static_cast<unsigned char>(text[index]);
    if (std::isdigit(ch) == 0) {
      fail(
          sourcePath,
          lineNumber,
          std::string{"invalid "} + std::string{label} + " index '" + std::string{text} + "'");
    }
    value = value * 10 + static_cast<int>(text[index] - '0');
  }

  if (value == 0) {
    fail(sourcePath, lineNumber, std::string{label} + " index 0 is invalid in OBJ");
  }

  return offset == 0 ? value : -value;
}

[[nodiscard]] std::size_t resolveIndex(
    int index,
    std::size_t count,
    const std::filesystem::path& sourcePath,
    std::size_t lineNumber,
    std::string_view label)
{
  if (index > 0) {
    const std::size_t resolved = static_cast<std::size_t>(index - 1);
    if (resolved < count) {
      return resolved;
    }
  } else {
    const int resolved = static_cast<int>(count) + index;
    if (resolved >= 0 && static_cast<std::size_t>(resolved) < count) {
      return static_cast<std::size_t>(resolved);
    }
  }

  fail(
      sourcePath,
      lineNumber,
      std::string{"out-of-range "} + std::string{label} + " index " + std::to_string(index) +
          " with " + std::to_string(count) + " available values");
}

[[nodiscard]] FaceVertex parseFaceVertex(
    std::string_view token,
    std::size_t positionCount,
    std::size_t texCoordCount,
    std::size_t normalCount,
    const std::filesystem::path& sourcePath,
    std::size_t lineNumber)
{
  FaceVertex vertex{};

  const std::size_t firstSlash = token.find('/');
  const std::size_t secondSlash = firstSlash == std::string_view::npos
                                      ? std::string_view::npos
                                      : token.find('/', firstSlash + 1);
  if (secondSlash != std::string_view::npos &&
      token.find('/', secondSlash + 1) != std::string_view::npos) {
    fail(sourcePath, lineNumber, std::string{"unsupported face token '"} + std::string{token} + "'");
  }

  const std::string_view positionText = token.substr(0, firstSlash);
  const int positionIndex = parseSignedIndex(positionText, sourcePath, lineNumber, "position");
  vertex.positionIndex =
      resolveIndex(positionIndex, positionCount, sourcePath, lineNumber, "position");

  if (firstSlash == std::string_view::npos) {
    return vertex;
  }

  const std::string_view texCoordText =
      token.substr(firstSlash + 1, secondSlash == std::string_view::npos ? std::string_view::npos
                                                                         : secondSlash - firstSlash - 1);
  if (!texCoordText.empty()) {
    vertex.hasTexCoord = true;
    const int texCoordIndex = parseSignedIndex(texCoordText, sourcePath, lineNumber, "texcoord");
    vertex.texCoordIndex =
        resolveIndex(texCoordIndex, texCoordCount, sourcePath, lineNumber, "texcoord");
  }

  if (secondSlash == std::string_view::npos) {
    return vertex;
  }

  const std::string_view normalText = token.substr(secondSlash + 1);
  if (!normalText.empty()) {
    vertex.hasNormal = true;
    const int normalIndex = parseSignedIndex(normalText, sourcePath, lineNumber, "normal");
    vertex.normalIndex =
        resolveIndex(normalIndex, normalCount, sourcePath, lineNumber, "normal");
  }

  return vertex;
}

[[nodiscard]] Vec3 parseNormal(
    std::istringstream& lineStream,
    const std::filesystem::path& sourcePath,
    std::size_t lineNumber)
{
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
  if (!(lineStream >> x >> y >> z)) {
    fail(sourcePath, lineNumber, "invalid normal line");
  }

  const Vec3 normal{x, y, z};
  const float normalLength = length(normal);
  if (normalLength <= kNormalEpsilon) {
    fail(sourcePath, lineNumber, "normal must not be zero-length");
  }

  return normal / normalLength;
}

[[nodiscard]] Triangle makeTriangle(
    const FaceVertex& a,
    const FaceVertex& b,
    const FaceVertex& c,
    const std::vector<Vec3>& positions,
    const std::vector<Vec2>& texCoords,
    const std::vector<Vec3>& normals,
    int materialId,
    const std::filesystem::path& sourcePath,
    std::size_t lineNumber)
{
  Triangle triangle{};
  triangle.v0 = positions[a.positionIndex];
  triangle.v1 = positions[b.positionIndex];
  triangle.v2 = positions[c.positionIndex];

  triangle.uv0 = a.hasTexCoord ? texCoords[a.texCoordIndex] : Vec2{};
  triangle.uv1 = b.hasTexCoord ? texCoords[b.texCoordIndex] : Vec2{};
  triangle.uv2 = c.hasTexCoord ? texCoords[c.texCoordIndex] : Vec2{};

  const bool hasAllNormals = a.hasNormal && b.hasNormal && c.hasNormal;
  if (hasAllNormals) {
    triangle.n0 = normals[a.normalIndex];
    triangle.n1 = normals[b.normalIndex];
    triangle.n2 = normals[c.normalIndex];
  } else {
    const Vec3 faceNormal = cross(triangle.v1 - triangle.v0, triangle.v2 - triangle.v0);
    const float faceNormalLength = length(faceNormal);
    if (faceNormalLength <= kNormalEpsilon) {
      fail(sourcePath, lineNumber, "degenerate face cannot generate a normal");
    }

    const Vec3 normalizedNormal = faceNormal / faceNormalLength;
    triangle.n0 = normalizedNormal;
    triangle.n1 = normalizedNormal;
    triangle.n2 = normalizedNormal;
  }

  triangle.materialId = materialId;
  triangle.computeCachedEdges();
  return triangle;
}

// Forward declarations for MTL parsing functions
[[nodiscard]] std::vector<MtlMaterial> parseMtlFile(
    const std::filesystem::path& mtlPath,
    const std::filesystem::path& baseDirectory);
void loadTextures(
    std::vector<Material>& materials,
    const std::vector<MtlMaterial>& mtlMaterials,
    const std::filesystem::path& baseDirectory);
Material convertMaterial(const MtlMaterial& mtlMat);

// Parse MTL file and return materials map
[[nodiscard]] std::vector<MtlMaterial> parseMtlFile(
    const std::filesystem::path& mtlPath,
    [[maybe_unused]] const std::filesystem::path& baseDirectory)
{
  std::vector<MtlMaterial> materials;
  std::ifstream input(mtlPath);
  if (!input) {
    return materials;
  }

  MtlMaterial currentMaterial;
  bool hasCurrentMaterial = false;

  std::string line;
  std::size_t lineNumber = 0;
  while (std::getline(input, line)) {
    ++lineNumber;

    const std::size_t comment = line.find('#');
    const std::string_view lineView = std::string_view{line}.substr(0, comment);
    const std::string trimmedLine = trim(lineView);
    if (trimmedLine.empty()) {
      continue;
    }

    std::istringstream lineStream(trimmedLine);
    std::string tag;
    lineStream >> tag;

    if (tag == "newmtl") {
      // Save previous material if exists
      if (hasCurrentMaterial) {
        materials.push_back(currentMaterial);
      }
      // Start new material
      currentMaterial = MtlMaterial{};
      std::getline(lineStream, currentMaterial.name);
      currentMaterial.name = trim(currentMaterial.name);
      hasCurrentMaterial = true;
      continue;
    }

    if (!hasCurrentMaterial) {
      continue;
    }

    if (tag == "Kd" || tag == "kd") {
      lineStream >> currentMaterial.albedo.x >> currentMaterial.albedo.y >> currentMaterial.albedo.z;
      continue;
    }

    if (tag == "Ke" || tag == "ke") {
      lineStream >> currentMaterial.emissionColor.x >> currentMaterial.emissionColor.y >> currentMaterial.emissionColor.z;
      continue;
    }

    if (tag == "Ns" || tag == "ns") {
      float specularExponent = 0.0F;
      lineStream >> specularExponent;
      // Convert specular exponent to roughness (approximate)
      currentMaterial.roughness = 1.0F - (specularExponent / 1000.0F);
      currentMaterial.roughness = std::clamp(currentMaterial.roughness, 0.0F, 1.0F);
      continue;
    }

    if (tag == "d" || tag == "Tr") {
      float transparency = 0.0F;
      lineStream >> transparency;
      currentMaterial.reflectivity = 1.0F - transparency;
      continue;
    }

    if (tag == "map_Kd" || tag == "map_Ka" || tag == "map_Ks") {
      // Albedo/diffuse texture
      std::string texturePath;
      std::getline(lineStream, texturePath);
      currentMaterial.albedoTexturePath = trim(texturePath);
      continue;
    }

    if (tag == "map_Metalness" || tag == "map_metallic" || tag == "map_Km" || tag == "map_refl") {
      std::string texturePath;
      std::getline(lineStream, texturePath);
      currentMaterial.metallicTexturePath = trim(texturePath);
      continue;
    }

    if (tag == "map_Pm" || tag == "map_roughness" || tag == "map_Pr" || tag == "map_d") {
      std::string texturePath;
      std::getline(lineStream, texturePath);
      currentMaterial.roughnessTexturePath = trim(texturePath);
      continue;
    }

    if (tag == "map_Ns") {
      // map_Ns (specular exponent map) can be used as roughness texture
      std::string texturePath;
      std::getline(lineStream, texturePath);
      currentMaterial.roughnessTexturePath = trim(texturePath);
      continue;
    }

    // Ignore bump/normal maps for now
    if (tag == "map_Bump" || tag == "bump") {
      continue;
    }
  }

  // Save last material
  if (hasCurrentMaterial) {
    // If Kd (diffuse) was not specified, default to white
    if (currentMaterial.albedo.x == 0.0F && currentMaterial.albedo.y == 0.0F && currentMaterial.albedo.z == 0.0F) {
      currentMaterial.albedo = Vec3{1.0F, 1.0F, 1.0F};
    }
    materials.push_back(currentMaterial);
  }

  return materials;
}

// Load textures for materials using stb_image
void loadTextures(
    std::vector<Material>& materials,
    const std::vector<MtlMaterial>& mtlMaterials,
    const std::filesystem::path& baseDirectory)
{
  const std::filesystem::path textureBaseDir = baseDirectory.parent_path();

  for (std::size_t i = 0; i < materials.size() && i < mtlMaterials.size(); ++i) {
    Material& mat = materials[i];
    const MtlMaterial& mtlMat = mtlMaterials[i];

    // Load albedo texture
    if (!mtlMat.albedoTexturePath.empty()) {
      const std::filesystem::path fullPath = textureBaseDir / mtlMat.albedoTexturePath;
      int width = 0, height = 0, channels = 0;
      unsigned char* data = stbi_load(fullPath.string().c_str(), &width, &height, &channels, 0);

      if (data != nullptr && width > 0 && height > 0) {
        mat.albedoTextureData.assign(data, data + width * height * channels);
        mat.albedoTextureWidth = width;
        mat.albedoTextureHeight = height;
        mat.albedoTextureChannels = channels;
        mat.hasAlbedoTexture = true;
        stbi_image_free(data);
      }
    }

    // Load metallic texture
    if (!mtlMat.metallicTexturePath.empty()) {
      const std::filesystem::path fullPath = textureBaseDir / mtlMat.metallicTexturePath;
      int width = 0, height = 0, channels = 0;
      unsigned char* data = stbi_load(fullPath.string().c_str(), &width, &height, &channels, 0);

      if (data != nullptr && width > 0 && height > 0) {
        mat.metallicTextureData.assign(data, data + width * height * channels);
        mat.metallicTextureWidth = width;
        mat.metallicTextureHeight = height;
        mat.metallicTextureChannels = channels;
        mat.hasMetallicTexture = true;
        stbi_image_free(data);
      }
    }

    // Load roughness texture
    if (!mtlMat.roughnessTexturePath.empty()) {
      const std::filesystem::path fullPath = textureBaseDir / mtlMat.roughnessTexturePath;
      int width = 0, height = 0, channels = 0;
      unsigned char* data = stbi_load(fullPath.string().c_str(), &width, &height, &channels, 0);

      if (data != nullptr && width > 0 && height > 0) {
        mat.roughnessTextureData.assign(data, data + width * height * channels);
        mat.roughnessTextureWidth = width;
        mat.roughnessTextureHeight = height;
        mat.roughnessTextureChannels = channels;
        mat.hasRoughnessTexture = true;
        stbi_image_free(data);
      }
    }
  }
}

// Convert MtlMaterial to Material
Material convertMaterial(const MtlMaterial& mtlMat)
{
  Material mat{};
  mat.albedo = mtlMat.albedo;
  mat.roughness = mtlMat.roughness;
  mat.metallic = mtlMat.metallic;
  mat.reflectivity = mtlMat.reflectivity;
  mat.emissionColor = mtlMat.emissionColor;
  mat.emissionStrength = mtlMat.emissionStrength;
  mat.albedoTexturePath = mtlMat.albedoTexturePath;
  mat.metallicTexturePath = mtlMat.metallicTexturePath;
  mat.roughnessTexturePath = mtlMat.roughnessTexturePath;
  return mat;
}

} // namespace

std::vector<Material> ObjLoader::loadMaterials(
    const std::filesystem::path& mtlPath,
    bool loadTexturesEnabled)
{
  std::vector<Material> materials;
  
  if (!std::filesystem::exists(mtlPath)) {
    return materials;
  }

  const std::vector<MtlMaterial> mtlMaterials = parseMtlFile(mtlPath, mtlPath);
  
  // Convert MTL materials to internal Material format
  materials.reserve(mtlMaterials.size());
  for (const auto& mtlMat : mtlMaterials) {
    materials.push_back(convertMaterial(mtlMat));
  }

  // Load textures if requested
  if (loadTexturesEnabled) {
    loadTextures(materials, mtlMaterials, mtlPath);
  }

  return materials;
}

Mesh ObjLoader::loadFromFile(const std::filesystem::path& path, const ObjLoadOptions& options)
{
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("Failed to open OBJ file: " + path.string());
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  Mesh mesh = loadFromString(buffer.str(), options, path);
  mesh.setSourcePath(path);
  return mesh;
}

Mesh ObjLoader::loadFromString(
    std::string_view source,
    const ObjLoadOptions& options,
    const std::filesystem::path& sourcePath)
{
  Mesh mesh{options.materialId};
  mesh.setSourcePath(sourcePath);

  std::vector<Vec3> positions;
  std::vector<Vec2> texCoords;
  std::vector<Vec3> normals;

  std::istringstream stream{std::string{source}};
  std::string line;
  std::size_t lineNumber = 0;
  while (std::getline(stream, line)) {
    ++lineNumber;

    const std::size_t comment = line.find('#');
    const std::string_view lineView = std::string_view{line}.substr(0, comment);
    const std::string trimmedLine = trim(lineView);
    if (trimmedLine.empty()) {
      continue;
    }

    std::istringstream lineStream(trimmedLine);
    std::string tag;
    lineStream >> tag;

    if (tag == "v") {
      float x = 0.0F;
      float y = 0.0F;
      float z = 0.0F;
      if (!(lineStream >> x >> y >> z)) {
        fail(sourcePath, lineNumber, "invalid vertex line");
      }
      positions.push_back(Vec3{x, y, z});
      continue;
    }

    if (tag == "vt") {
      float u = 0.0F;
      float v = 0.0F;
      if (!(lineStream >> u >> v)) {
        fail(sourcePath, lineNumber, "invalid texcoord line");
      }
      texCoords.push_back(Vec2{u, v});
      continue;
    }

    if (tag == "vn") {
      normals.push_back(parseNormal(lineStream, sourcePath, lineNumber));
      continue;
    }

    if (tag == "o") {
      mesh.setName(trim(trimmedLine.substr(1)));
      continue;
    }

    if (tag == "g") {
      mesh.setGroupName(trim(trimmedLine.substr(1)));
      continue;
    }

    if (tag == "usemtl") {
      mesh.setSourceMaterialName(trim(trimmedLine.substr(6)));
      continue;
    }

    if (tag == "mtllib") {
      mesh.setMaterialLibrary(trim(trimmedLine.substr(6)));
      continue;
    }

    if (tag == "f") {
      std::vector<FaceVertex> faceVertices;
      std::string token;
      while (lineStream >> token) {
        faceVertices.push_back(parseFaceVertex(
            token,
            positions.size(),
            texCoords.size(),
            normals.size(),
            sourcePath,
            lineNumber));
      }

      if (faceVertices.size() < 3U) {
        fail(sourcePath, lineNumber, "face must contain at least 3 vertices");
      }

      for (std::size_t index = 1; index + 1 < faceVertices.size(); ++index) {
        mesh.addTriangle(makeTriangle(
            faceVertices[0],
            faceVertices[index],
            faceVertices[index + 1],
            positions,
            texCoords,
            normals,
            options.materialId,
            sourcePath,
            lineNumber));
      }
    }
  }

  return mesh;
}

} // namespace astraglyph
