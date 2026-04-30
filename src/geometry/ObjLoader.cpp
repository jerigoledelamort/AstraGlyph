#include "geometry/ObjLoader.hpp"

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
  return triangle;
}

} // namespace

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
