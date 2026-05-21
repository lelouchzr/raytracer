# Binary releases

| Package | Build target | Included |
|---|---|---|
| Linux x86_64 | Ubuntu 22.04, GCC 12; glibc ≥ 2.35 | CLI, SDL2/ImGui studio, SFML viewer, example plugin, scenes/assets |
| macOS ARM64 | macOS 15+, Apple Silicon | Same features plus `Atelier.app` |

SDL2, SFML and zlib runtimes are installed separately. Exact dynamic dependencies and file hashes are recorded in each archive's `manifest.json`. The macOS app is unsigned and not notarized.

## Use an archive

```sh
tar -xzf raytracer-v3.0.0-PLATFORM.tar.gz
cd raytracer-v3.0.0-PLATFORM
./raytracer --studio
./raytracer --preview bonus/scenes/pbr_showcase.json
```

Keep the archive directory together. The default studio scene, examples menu, OBJ preset and HDR preset locate resources beside the executable or inside the app bundle. `RAYTRACER_RESOURCE_ROOT` can select another resource root. Explicit scene arguments and output paths retain their normal working-directory semantics.

## Build and verify

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DRAYTRACER_REQUIRE_GUI=ON
cmake --build build-release --parallel 4
ctest --test-dir build-release --output-on-failure
python3 tools/package_release.py build-release --version v3.0.0
python3 tests/release_package.py dist/*.tar.gz
# Linux: xvfb-run -a python3 tests/release_package.py dist/*.tar.gz --gui
```

Use a clean checkout at the release tag. On macOS add `-DCMAKE_OSX_DEPLOYMENT_TARGET=15.0`. The **Release binaries** workflow builds both platforms and uploads validated archives/checksums as artifacts. Releases are prepared from those exact artifacts; the manifest source SHA must match the annotated tag.

Verification covers checksums, extraction into a path containing spaces, CLI errors, four integrators, deterministic threading, plugin loading, OBJ/HDR assets, default native studio resources and the macOS app. Linux additionally exercises actual SDL2 and SFML windows under Xvfb.

The project uses [MIT](../LICENSE), with [third-party notices](../THIRD_PARTY_NOTICES.md) and Dear ImGui's license preserved. Generated diagnostics and build outputs are excluded from source history and binary archives.
