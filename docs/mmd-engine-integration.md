# License-Safe MMD Asset Pipeline

## Recommendation

Use MMD Tools as an optional, separately installed content-authoring tool. Do
not vendor, copy, link, bundle, or make the game executable depend on
`blender_mmd_tools`. Import MMD assets in Blender and export a glTF 2.0 GLB;
the engine should load that interchange asset through its own glTF runtime.

This keeps the current MIT engine source separate from the GPL add-on and lets
users who already use Blender continue to author MMD content. It does not
change the license of any model, motion, texture, or other input asset; users
must follow those assets' own terms.

## Upstream and license review

- The user-linked [`powroupi/blender_mmd_tools`](https://github.com/powroupi/blender_mmd_tools)
  repository says it is no longer maintained and points to the current
  [`MMD-Blender/blender_mmd_tools`](https://github.com/MMD-Blender/blender_mmd_tools).
- The current add-on declares **GPL-3.0-or-later** and supports PMX/PMD model
  plus VMD/VPD motion/pose import/export. Treat its Python implementation as
  GPL-covered; do not copy implementation code, tests, or bundled assets into
  this MIT repository.
- If the project ever decides to ship or link the add-on, stop and make a
  separate distribution-level license decision first. This pipeline proposal
  intentionally does not do that.
- The [`cgltf`](https://github.com/jkuhlmann/cgltf) parser is a possible future
  runtime dependency and publishes an MIT license. If selected and vendored,
  pin a reviewed revision and add its exact copyright/license notice to
  `THIRD_PARTY_NOTICES.md` before merging it.
- glTF is an interchange specification, not a blanket license for a model.
  Keep the original creator's asset license and attribution alongside each
  converted asset.

## Engine fit

The current renderer draws CPU-projected triangles and has a flat-triangle PBR
slice. The engine does not yet load indexed mesh assets or provide a complete
depth-tested mesh, texture, skeletal skinning, morph-target, or animation
runtime. Therefore MMD Tools cannot be dropped into the runtime as a renderer
feature; the engine needs an asset and animation path first.

Recommended implementation order:

1. Add a glTF 2.0/GLB asset loader behind the existing Data/Rendering boundary,
using a license-reviewed MIT parser such as `cgltf`. Start with validated
indexed static meshes, node transforms, and core metallic-roughness materials.
2. Add the depth-tested indexed mesh pass described in
   [`rendering-technology-review.md`](rendering-technology-review.md), then
   support texture loading and image lifetime through the existing asset
   cache.
3. Add glTF skinning and animation, then morph targets. This enables the
   Blender exporter to carry bones, baked animation, and shape keys that it
   can represent in glTF.
4. Validate the conversion with creator-authorized PMX/PMD + VMD samples.
   Compare source and GLB in Blender and the engine, and record unsupported
   features and any lossy conversion.

Do not begin with an in-engine PMX/VMD parser. PMX has multiple versions and
deform/morph/physics variants; VMD documentation is community reverse
engineering rather than a complete official specification. If direct PMX/VMD
runtime support later becomes a requirement, implement it independently from
format facts, preserve provenance for every reference, and review the source
and documentation licenses before adopting any code or substantial text.

## Authoring workflow and fidelity limits

1. A developer installs a compatible Blender release and MMD Tools from the
   official Blender Extensions platform. Neither installer nor add-on is
   included in StonePotatoes packages or CI.
2. Import a PMX/PMD model into Blender, then apply VMD motion to that model.
3. Bake Blender-evaluated motion when the source uses MMD-specific IK or
   physics behavior that the engine does not implement.
4. Export GLB with mesh, materials, skinning, animation, and shape keys enabled
   where supported. Keep a source-to-output manifest and the original asset
   license with the converted files.
5. Load the GLB through the engine's future glTF runtime and run a visual
   regression scene.

Conversion is not guaranteed to preserve every MMD visual feature. Toon and
sphere-map shading, outlines, PMX material flags, IK semantics, and rigid-body
simulation may need baking or an engine-specific extension. The first runtime
slice should report these losses instead of silently promising full MMD
compatibility.

## References

- [User-linked, unmaintained fork](https://github.com/powroupi/blender_mmd_tools)
- [Maintained MMD Tools repository and GPL-3.0-or-later license](https://github.com/MMD-Blender/blender_mmd_tools)
- [MMD Tools Blender Extensions listing and version/license metadata](https://extensions.blender.org/add-ons/mmd-tools/versions/)
- [Blender 4.4 glTF exporter: animation, skinning, and shape keys](https://docs.blender.org/manual/en/4.4/addons/import_export/scene_gltf2.html)
- [cgltf MIT license](https://github.com/jkuhlmann/cgltf/blob/master/LICENSE)
- [Khronos glTF project copying and trademark notice](https://github.com/KhronosGroup/glTF/blob/main/COPYING.adoc)
- [PMX 2.1 community format notes](https://github.com/dskjal/OpenMMD/blob/main/docs/specification/pmx2.1-specification.md)
- [VMD/VPD format background and reverse-engineering caveat](https://noname0310.github.io/babylon-mmd/docs/reference/understanding-mmd-behaviour/introduction-to-vmd-and-vpd/)

This is a project engineering and license-provenance review, not legal advice.
