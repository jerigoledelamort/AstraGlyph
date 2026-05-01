cmake_minimum_required(VERSION 3.25)

project(
  AstraGlyph
  VERSION 0.1.0
  LANGUAGES CXX
)

option(ASTRAGLYPH_ENABLE_TESTS "Build AstraGlyph tests" ON)
option(ASTRAGLYPH_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Optimization flags for Release builds
if(NOT MSVC)
  add_compile_options($<$<CONFIG:Release>:-O3>)
else()
  add_compile_options($<$<CONFIG:Release>:/O2 /fp:fast /arch:AVX2>)
endif()

include(ProjectOptions)
include(CompilerWarnings)
include(Dependencies)
include(Sanitizers)

astraglyph_setup_project_options()
astraglyph_find_dependencies()

set(ASTRAGLYPH_ENGINE_SOURCES
  src/app/Application.cpp
  src/platform/Window.cpp
  src/platform/Input.cpp
  src/platform/Time.cpp
  src/core/Log.cpp
  src/core/ThreadPool.cpp
  src/scene/Scene.cpp
  src/scene/SceneLoader.cpp
  src/scene/Camera.cpp
  src/scene/Mesh.cpp
  src/geometry/Triangle.cpp
  src/geometry/MeshFactory.cpp
  src/geometry/ObjLoader.cpp
  src/render/Renderer.cpp
  src/render/AsciiMapper.cpp
  src/render/Sampler.cpp
  src/render/RayTracer.cpp
  src/render/Integrator.cpp
  src/render/Bvh.cpp
  src/ui/DebugOverlay.cpp
  src/ui/SettingsPanel.cpp
)

add_library(astraglyph_engine STATIC ${ASTRAGLYPH_ENGINE_SOURCES})
target_include_directories(astraglyph_engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(astraglyph_engine SYSTEM PUBLIC "${ASTRAGLYPH_JSON_INCLUDE_DIR}")
target_link_libraries(astraglyph_engine PUBLIC ${ASTRAGLYPH_SDL3_TARGET})
astraglyph_apply_project_options(astraglyph_engine)
astraglyph_set_warnings(astraglyph_engine ${ASTRAGLYPH_WARNINGS_AS_ERRORS})
astraglyph_enable_sanitizers(astraglyph_engine)

add_executable(astraglyph src/app/main.cpp)
target_link_libraries(astraglyph PRIVATE astraglyph_engine)
astraglyph_apply_project_options(astraglyph)
astraglyph_set_warnings(astraglyph ${ASTRAGLYPH_WARNINGS_AS_ERRORS})
astraglyph_enable_sanitizers(astraglyph)

if(ASTRAGLYPH_ENABLE_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
