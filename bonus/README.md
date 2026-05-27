# Raytracer extensions

Enabled by default. `make BUILD=build-core BONUS=OFF` builds the core; `make` restores bonuses. Headers in `include/rt/`, methods in `src/`, within this directory.

| Directories | Contents |
|---|---|
| `geometry`, `math`, `transform`, `acceleration` | Finite shapes, quartics, fractal, Möbius, matrices, groups, BVH |
| `io` | OBJ/MTL, images, RGBE, legacy formats |
| `materials`, `lights`, `render`, `pbr` | Textures, Phong, path tracing, photons, GGX/MIS, HDR, workers |
| `studio`, `display` | SDL2/ImGui studio, SFML viewer |
| `plugins`, `network`, `scene` | ABI 3, leases, TCP RTR1, builder extensions |

`plugins/`: complete example; `assets/`: bundled resources; `scenes/`: demonstrations; `render_gallery.py`: regenerate the selected root `gallery/`.

[Features/evidence](../doc/feature-matrix.md) · [Studio/PBR](../doc/pbr-studio.md) · [Optical trade-offs](../doc/optics.md) · [Tests](../doc/validation.md).
