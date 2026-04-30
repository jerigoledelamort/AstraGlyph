# AstraGlyph / AsciiTracer

Initial C++20 scaffold for the AstraGlyph / AsciiTracer renderer.

## Architecture rules

- An ASCII symbol is the smallest image unit.
- The renderer must not create a pixel framebuffer inside a symbol cell.
- All render geometry flows through Mesh and Triangle data.
- Primitive shapes and OBJ assets must be converted into the shared Mesh/Triangle pipeline.
- RayTracer has no dependency on SDL, windows, UI, or ASCII output.
- Renderer has no dependency on SDL internals.
- Scene has no dependency on UI.

## Build

Out-of-source builds are configured through CMake presets:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

Release:

```sh
cmake --preset release
cmake --build --preset release
```
