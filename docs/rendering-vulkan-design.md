# Rendering backend decision: shared Vulkan context

Status: shared-context Vulkan Renderer complete; perspective 3D triangles use CPU projection, near/far plane clipping, and painter sorting; Linux CI runs Xvfb checks for shared-device presentation and resize-driven swapchain recreation

## Decision

Build the windowed renderer on a shared internal Vulkan context used by both
`StonePotatoesGPU` compute and `StonePotatoesRendering` graphics. Keep Vulkan
types out of the installed C++ API. Do not grow the current OpenGL renderer into
a second long-term backend.

The first implementation slice extracts instance/device/compute-queue ownership
from `GPU::VulkanBackend` into an internal `GPU::VulkanContext`. The compute
backend and future graphics renderer can share this context; neither should
create or destroy the shared instance or device. Vulkan-enabled software-device
CI compiles and exercises the compute path, while a headless CPU-only build
remains supported when the Vulkan SDK is unavailable. A focused internal
`VulkanPresentationSmoke` creates a GLFW surface, swapchain, and clear/present
frames. The production `Renderer` now uses this presentation owner with ImGui's
Vulkan backend and maps its existing point/circle API to ImGui draw primitives.
CPU-only builds keep the OpenGL fallback because Vulkan remains optional.

Phase 2 separates instance creation from device selection. Callers may provide
required instance extensions before creating a window surface, then pass the
non-owning surface to device initialization. A presentation-capable device must
provide compute and graphics queues, surface presentation support, and
`VK_KHR_swapchain`; unique queue families are requested once. The smoke helper
owns the window, surface, swapchain, render pass, and frame synchronization,
while borrowing the context. The Vulkan renderer returns its context to compute
consumers; a renderer-supplied context must be uninitialized so the required
window-system extensions can be enabled before device selection.

## Initial 3D geometry slice

`Renderer::SetCamera3D` configures a right-handed perspective camera and
`Renderer::DrawTriangle3D` queues colored world-space triangles. At frame end,
the shared rendering layer transforms triangles to view space, clips them
against all six camera-frustum planes, triangulates the remaining polygon,
projects the visible faces, and draws opaque faces back-to-front through ImGui. Fully
outside and degenerate geometry is rejected. This keeps the API available in
both Vulkan and CPU-only OpenGL builds and does not add Vulkan types to the
public interface.

This is a bootstrap for 3D visualization, not a general mesh renderer. Painter
sorting works for separated or non-intersecting opaque surfaces; intersecting
surfaces, order-independent transparency, lighting, materials, GPU mesh
buffers, and depth-buffer testing require the next renderer phase. The
`renderer3d_geometry_unit` test covers camera validation, perspective
projection, near/far and viewport-side clipping, depth order, and per-frame
geometry reset. The Vulkan
compute/renderer demo includes a tetrahedron so the Linux Xvfb runtime check
exercises this path.

## Atlas-based 2D sprites

`Renderer::CreateTexture2D` accepts caller-provided RGBA8 pixels and returns an
opaque renderer-owned handle. `DrawSprite2D` queues world-space quads with
normalized atlas UVs, size, rotation, tint, and a stable painter layer. The
renderer sorts by layer and sends quads through Dear ImGui's dynamic texture
support, which is implemented by both the Vulkan and OpenGL2 backends. Sprites
using the same atlas share a texture and are batched by the draw-list backend.
Texture destruction is deferred through the backend's texture lifecycle so
in-flight frames can release their references. `SpriteAnimation2D` advances
atlas UV clips by explicit integer ticks and supports looping or one-shot
playback; it stays presentation-side so simulation timing remains independent.
Image-file decoding, atlas packing, and a dedicated GPU instancing pipeline
remain later work. The compute/renderer demo animates a sprite from a
procedural 2x2 atlas through the integrated runtime path.

## Why an extraction is required

`VulkanContext` now owns the `VkInstance`, `VkDevice`, and compute queue, and
`VulkanBackend` exposes a shared internal context handle. A headless context
created before GLFW's window-system extensions are known cannot later create a
presentation surface; the windowed owner must provide those extensions before
instance creation and initialize the device after creating its surface.

The CPU-only fallback in `src/rendering/Renderer.cpp` uses GLFW, OpenGL 2.1, and
ImGui's OpenGL2 backend. Vulkan-enabled builds compile the ImGui Vulkan backend
and avoid OpenGL linkage for `StonePotatoesRendering`.

## Ownership and initialization

1. Initialize GLFW and obtain its required Vulkan instance extensions before
   creating the context. The context enables those extensions only when a
   windowed renderer is requested; headless compute initialization stays valid.
2. Create the Vulkan instance, then let the presentation owner create its GLFW
   window and `VkSurfaceKHR` using that instance.
3. Complete device selection after the surface exists. A windowed device must
   support compute, graphics, and presentation to that surface. Queue families
   may differ; request each unique family once and serialize submissions to
   each queue. Headless compute selection does not require a surface.
4. `VulkanContext` owns the instance, physical device, logical device, and
   queue handles. It does not own a GLFW window or surface; the surface is
   passed as a non-owning device-selection requirement.
5. The presentation owner owns its GLFW window, Vulkan surface, swapchain,
   image views, render pass, framebuffers, and presentation synchronization. It
   borrows the context and destroys those resources before releasing it.
6. Compute buffers, graphics resources, and swapchain recreation must use the
   shared device. Public consumer headers continue to use opaque engine types.

## Delivery sequence

1. [Complete] Extract context ownership without changing compute behavior;
   software-device CI exercises the existing compute path and CPU-only builds
   remain supported.
2. [Validated locally] Context capability negotiation and clear/present smoke
   path are complete. A local Vulkan-device run survived maximizing the window,
   exercising resize-driven swapchain recreation. The production visualization
   demo also initialized the Vulkan Renderer, ran its simulation/render loop,
   and exited cleanly after a normal window-close request on an RTX 5050.
   The integrated demo now verifies same-device compute/presentation locally.
3. [Integrated; local runtime validated] The production Renderer uses ImGui's
   Vulkan backend and maps the existing point/circle API to 2D draw primitives.
   The renderer exposes the shared context for compute consumers. The
   `StonePotatoesVulkanComputeRendererDemo` now dispatches SAXPY through a
   `VulkanBackend` bound to that context, verifies the result and device handle,
   and visualizes the output. The target builds locally; an interactive run
   verified the shared device, all 64 SAXPY values, and normal window-close
   shutdown (exit code 0). Linux Vulkan CI is configured to run the demo for
   three frames under Xvfb and fail if compute or frame presentation fails. A
   separate Xvfb smoke test resizes the surface, checks that the swapchain
   generation advances, presents another frame, and shuts down cleanly.
4. [Initial 3D slice] The renderer exposes a perspective camera and 3D triangle
   API shared by Vulkan and OpenGL fallback builds. Depth sorting is a bootstrap
   only; implement a GPU mesh pipeline with a real depth attachment before
   claiming general 3D scene support.
5. Remove the OpenGL2 fallback after Vulkan is no longer optional for supported
   build configurations.

## Acceptance

- CPU-only configure/build/tests remain supported with Vulkan discovery
  disabled.
- Vulkan-enabled tests prove compute still works through the extracted shared
  context.
- A windowed smoke test creates one context, runs compute and presents graphics
  through the same `VkDevice`, resizes the surface, and shuts down cleanly. The
  presentation smoke verified resize handling; the integrated demo verified the
  shared `VkDevice`, 64 compute results, and normal shutdown. Linux software-
  device CI runs both the three-frame integrated demo and a separate automated
  resize/recreate/present smoke under Xvfb.
- The installed headless C++ package does not gain a GLFW or Vulkan dependency.
- No second Vulkan instance/device is created by `Renderer` when a shared
  context is supplied.
