# 2D/3D Rendering Technology Review

> Review date: 2026-10-19. Discussion: [Rendering roadmap: modern 2D and 3D techniques](https://github.com/ArchaonHW/StonePotatoes/discussions/81).

## Current project baseline

The renderer presents through a shared Vulkan context when Vulkan is available,
with a CPU-only OpenGL fallback. Its 2D path has atlas-backed sprites, stable
layer ordering, and animation. Its 3D path currently CPU-projects and
painter-sorts triangles; an opt-in metallic-roughness PBR slice evaluates
direct lighting per flat triangle. This makes visibility correctness and
per-pixel shading the next 3D gaps, while repeated sprite submission is the
potential 2D scaling opportunity.

## Decision proposal

### 3D first: depth-tested indexed mesh pass

Build an optional Vulkan scene pass around vertex/index buffers, a depth
attachment, and dynamic rendering. This directly addresses intersecting and
overlapping surfaces, which painter sorting cannot render correctly, and
provides the right place for the existing PBR model to run in a fragment
shader. Keep Vulkan handles internal and keep CPU-only builds supported.

Prefer core Vulkan 1.3 dynamic rendering as the baseline. Vulkan 1.4 guidance
recommends `vkCmdBeginRendering` / `vkCmdEndRendering` over render-pass objects
for new code. Capability checks and the project's minimum Vulkan version still
need to be decided before implementation.

Start with opaque static meshes and a depth attachment. Add GPU PBR in a
follow-up after the depth-tested pass is validated. Defer image-based lighting,
HDR tone mapping, glTF asset loading, mesh shaders, task shaders, and
device-generated commands until there is a representative scene and measured
need.

### 2D next: instanced atlas sprites

The next 2D scaling slice should retain the current atlas and public sprite API,
but upload per-sprite transform, UV rectangle, tint, and layer data as instance
records. Draw a shared indexed quad for each compatible batch and use instanced
draws; preserve stable layer order and keep the current OpenGL fallback.
Establish a sprite-count/frame-time baseline first. Add indirect multi-draw,
texture arrays, or KTX2/Basis Universal only if profiling shows the first
instanced path is insufficient.

### Deferred options

| Technology | Potential value | Why it is deferred |
|---|---|---|
| Vulkan mesh/task shaders | Flexible GPU geometry generation and meshlet culling | Optional hardware/driver feature; Vulkan guidance warns it is not automatically faster than the traditional vertex pipeline. |
| GPU-generated indirect commands | Reduce CPU draw submission for large scenes | The project has no measured draw-call pressure or scene size to justify added synchronization and capability paths. |
| KTX2/Basis Universal | Compressed, transcodable texture distribution and streaming | Useful after image/asset loading is in scope; decoder choice needs a license, platform, and package-size review. |
| Ray tracing | Specialized reflections, shadows, or GI | No current scene or performance target warrants an optional hardware path. |

## Legal and portability constraints

- Implement against published API/format contracts; do not copy tutorial or
  sample source into engine files.
- Do not add third-party runtime dependencies until the selected component,
  version, license, notices, supported platforms, and maintenance state have
  been reviewed. Update `THIRD_PARTY_NOTICES.md` when code is adopted.
- Query Vulkan features and limits. The core raster path must not require
  mesh-shader or ray-tracing support.
- Keep the headless installed C++ package independent of GLFW and Vulkan.
- Benchmark the Vulkan path against representative workloads before claiming a
  performance improvement; feature availability alone is not evidence of a
  speedup.

## References

- [Khronos Vulkan 1.4 specification](https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html)
- [Vulkan dynamic rendering tutorial](https://docs.vulkan.org/tutorial/latest/courses/18_Ray_tracing/01_Dynamic_rendering.html)
- [Vulkan depth buffering tutorial](https://docs.vulkan.org/tutorial/latest/07_Depth_buffering.html)
- [Vulkan instanced scene rendering](https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Loading_Models/07_scene_rendering.html)
- [VK_EXT_mesh_shader feature proposal](https://docs.vulkan.org/features/latest/features/proposals/VK_EXT_mesh_shader.html)
- [KTX 2.0 specification](https://github.com/KhronosGroup/KTX-Specification/blob/main/ktxspec.adoc)
