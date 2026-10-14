# Data assets

`StonePotatoes::Data::AssetCache` loads binary or text files below a configured
asset root. Include `<data/AssetCache.h>` and link `StonePotatoes::Data` from
the installed CMake package.

```cpp
StonePotatoes::Data::AssetCache assets("assets");
auto textureBytes = assets.LoadBinary("textures/stone.bin");
auto description = assets.LoadText("scenes/intro.json");
```

Asset paths are relative to the root. Absolute paths and paths that escape the
root are rejected, including paths that resolve through symlinks outside the
root. Binary and text loads for one path share a cached byte buffer; repeated
loads return shared immutable handles. Reads are synchronous. A changed file
remains cached until `Invalidate(path)` or `Clear()`; already returned handles
remain valid after invalidation.

Text loads expose file bytes as a `std::string`; callers decide the text
encoding and parsing rules. Failed reads are not cached. The cache does not
watch files, decode image/audio formats, or provide asynchronous streaming.
