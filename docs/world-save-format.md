# World save format v1

`ECS::World::SaveToFile` writes the version 1 binary format. The header uses
little-endian integers; component payloads are the registered component's raw
object bytes.

| Field | Encoding |
|---|---|
| Magic | u32 `0x53505744` (`SPWD`) |
| Format version | u32 `1` |
| Entity count | u32 |
| Per entity | u32 index, u32 generation, u32 component count |
| Per component | u16 UTF-8 name length, name bytes, u32 payload size, payload bytes |

Only named-registered, trivially copyable components are written. Loading skips
components unknown to the current application or whose registered size differs.
The file is rejected if its header/version is unsupported, a record is
truncated, an entity or component is duplicated, a generation is zero, or
trailing bytes remain. The full file is validated before the destination world
is changed.

Version 1 payloads are ABI-specific: raw component bytes do not provide
cross-compiler, cross-architecture, or schema migration guarantees. Keep
component layouts stable for existing saves or introduce a new format version
with explicit field encodings and migration code before changing them. A failed
write may leave a partial file; atomic replacement is not yet provided.
