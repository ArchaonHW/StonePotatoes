# Third-Party Notices

This project incorporates code derived from PotatoEngine (D:\HWC\PotatoEngine), which is licensed under the MIT License. The following components are ports from PotatoEngine:

## PotatoEngine (MIT License)

- **Source**: D:\HWC\PotatoEngine
- **License**: MIT
- **Copyright**: (c) 2026 ArchaonHW

### Ported Components

#### Core/Profiler.{h,cpp} → src/core/profiling/Profiler.{h,cpp}
- **Original**: PotatoEngine/Core/Profiler.{h,cpp}
- **Ported to**: StonePotatoes/src/core/profiling/Profiler.{h,cpp}
- **Changes**:
  - Renamed namespace from `Potato` to `StonePotatoes::Core`
  - Renamed macro from `POTATO_PROFILE_SCOPE` to `STONEPOTATOES_PROFILE_SCOPE`
  - Adapted to StonePotatoes naming conventions (e.g., `m_` member prefix)
  - Added SPDX identifier and provenance header

#### MathUtils/Vector2.h → src/math/Vec2.h
- **Original**: PotatoEngine/MathUtils/Vector2.h
- **Ported to**: StonePotatoes/src/math/Vec2.h
- **Changes**:
  - Renamed namespace from `Potato` to `StonePotatoes::Math`
  - Minimal subset (removed unused methods, kept core operations)
  - Added SPDX identifier and provenance header

#### MathUtils/Vector3.h → src/math/Vec3.h
- **Original**: PotatoEngine/MathUtils/Vector3.h
- **Ported to**: StonePotatoes/src/math/Vec3.h
- **Changes**:
  - Renamed namespace from `Potato` to `StonePotatoes::Math`
  - Minimal subset (removed unused methods, kept core operations)
  - Added SPDX identifier and provenance header

#### MathUtils/Pathfinding.h → src/math/Pathfinding.h
- **Original**: PotatoEngine/MathUtils/Pathfinding.h
- **Ported to**: StonePotatoes/src/math/Pathfinding.h
- **Changes**:
  - Renamed namespace from `Potato` to `StonePotatoes::Math`
  - Adapted to StonePotatoes coding style (e.g., `m_` member prefix)
  - Added SPDX identifier and provenance header

#### MathUtils/QuasiRandom.h → src/math/QuasiRandom.h
- **Original**: PotatoEngine/MathUtils/QuasiRandom.h
- **Ported to**: StonePotatoes/src/math/QuasiRandom.h
- **Changes**:
  - Renamed namespace from `Potato::Quasi` to `StonePotatoes::Math::Quasi`
  - Changed `Vector2` to `StonePotatoes::Math::Vec2`
  - Added SPDX identifier and provenance header

#### MathUtils/CurlNoise.h → src/math/CurlNoise.h
- **Original**: PotatoEngine/MathUtils/CurlNoise.h
- **Ported to**: StonePotatoes/src/math/CurlNoise.h
- **Changes**:
  - Renamed namespace from `Potato::Quasi` to `StonePotatoes::Math::Quasi`
  - Changed `Vector3` to `StonePotatoes::Math::Vec3`
  - Adapted to StonePotatoes coding style (e.g., `m_` member prefix)
  - Added SPDX identifier and provenance header

#### Simulation/SystemDynamics.{h,cpp} → src/civilization/economic/SystemDynamics.{h,cpp}
- **Original**: PotatoEngine/Simulation/SystemDynamics.{h,cpp}
- **Ported to**: StonePotatoes/src/civilization/economic/SystemDynamics.{h,cpp}
- **Changes**:
  - Renamed namespace from `Potato::Simulation` to `StonePotatoes::Civilization::Economic`
  - Adapted to StonePotatoes coding style (e.g., `m_` member prefix)
  - Fixed `uint32` → `uint32_t` for MSVC compatibility
  - Kept discrete event stock deltas independent of the flow integration timestep
  - Added SPDX identifier and provenance header

#### Networking/LockstepSync.{h,cpp} → src/networking/core/LockstepSync.{h,cpp}
- **Original**: PotatoEngine/Networking/LockstepSync.{h,cpp}
- **Ported to**: StonePotatoes/src/networking/core/LockstepSync.{h,cpp}
- **Changes**:
  - Renamed namespace from `Potato` to `StonePotatoes::Networking`
  - Adapted to StonePotatoes coding style (e.g., `m_` member prefix)
  - Made `EncodeCommandPacket` public (was private, needed for testing)
  - Added SPDX identifier and provenance header

### PotatoEngine License Text

```
MIT License

Copyright (c) 2026 ArchaonHW

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## Other Third-Party Dependencies

This project may include other third-party dependencies with their own licenses. Refer to the `external/` directory and individual package licenses for details.
