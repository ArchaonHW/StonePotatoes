# Rendering backend decision: shared Vulkan context

Status: Phase 1 implementation in progress for Issue #10

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
remains supported when the Vulkan SDK is unavailable. Window-system extensions,
surface selection, and graphics queue negotiation remain future work.

## Why an extraction is required

`VulkanBackend::Impl` currently owns its `VkInstance`, `VkDevice`, compute queue,
and command pool. It creates the instance without window-system extensions,
prefers a dedicated compute-only queue, and exposes no context handle. GLFW
therefore cannot create a presentation surface for that instance, and a second
renderer-owned device would not share GPU resources with compute.

`src/rendering/Renderer.cpp` currently uses GLFW, OpenGL 2.1 immediate-mode
calls, and ImGui's OpenGL2 backend. Reusing that path would preserve the split
device/API problem Issue #10 calls out.

## Ownership and initialization

1. Initialize GLFW and obtain its required Vulkan instance extensions before
   creating the context. The context enables those extensions only when a
   windowed renderer is requested; headless compute initialization stays valid.
2. Create the Vulkan instance, then let `Renderer` create its GLFW window and
   `VkSurfaceKHR` using that instance.
3. Complete device selection after the surface exists. A windowed device must
   support compute, graphics, and presentation to that surface. Queue families
   may differ; request each unique family once and serialize submissions to
   each queue. Headless compute selection does not require a surface.
4. `VulkanContext` owns the instance, physical device, logical device, and
   queue handles. It does not own a GLFW window or surface; the surface is
   passed as a non-owning device-selection requirement.
5. `Renderer` owns its GLFW window, Vulkan surface, swapchain, image views,
   render pass, framebuffers, and presentation synchronization. It borrows the
   context and destroys those resources before the context is shut down.
6. Compute buffers, graphics resources, and swapchain recreation must use the
   shared device. Public consumer headers continue to use opaque engine types.

## Delivery sequence

1. [In progress] Extract context ownership without changing compute behavior;
   compile and exercise the existing compute path in software-device CI while
   preserving CPU-only builds.
2. Add windowed surface/device capability negotiation and a minimal Vulkan
   graphics smoke path that clears and presents a frame.
3. Move ImGui to its Vulkan backend and port the existing 2D point/circle
   drawing API, including resize and swapchain recreation.
4. Remove OpenGL2 linkage and the OpenGL2 ImGui backend once the Vulkan path
   covers the existing renderer API.

## Acceptance

- CPU-only configure/build/tests remain supported with Vulkan discovery
  disabled.
- Vulkan-enabled tests prove compute still works through the extracted shared
  context.
- A windowed smoke test creates one context, runs compute and presents graphics
  through the same `VkDevice`, resizes the surface, and shuts down cleanly.
- The installed headless C++ package does not gain a GLFW or Vulkan dependency.
- No second Vulkan instance/device is created by `Renderer` when a shared
  context is supplied.
