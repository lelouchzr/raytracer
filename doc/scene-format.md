# Scene reference

Strict JSON: no comments, trailing commas, or duplicate keys. Bonus builds import legacy/libconfig formats; [conventions](../tests/legacy/README.md). Modern coordinates: **Y up**, angles in degrees, finite numeric values ±10⁹.

## Minimal example

```json
{
  "camera": {"position": [0, 2, 6], "target": [0, 1, 0], "fov": 45},
  "render": {"width": 640, "height": 400, "output": "output.ppm"},
  "materials": {"red": {"color": [0.8, 0.1, 0.05]}},
  "objects": [{"type": "sphere", "radius": 1, "translate": [0, 1, 0], "material": "red"}],
  "lights": [
    {"type": "ambient", "intensity": 0.15},
    {"type": "directional", "direction": [-1, -2, -1], "intensity": 1}
  ]
}
```

`camera` and at least one object are required; lights are optional. Linear colors [0,1], emission [0,100]. Zero directions, unknown keys, or missing references produce stderr diagnostics and exit **84**.

## Root fields

| Field | Value / default |
|---|---|
| `camera.position` | `[0,2,7]` |
| `camera.target` | `[0,1,0]`, look-at point, different from position |
| `camera.up` | `[0,1,0]`, not collinear with view axis |
| `camera.fov` | 45°, vertical angle 1..175 |
| `background` | `[0.025,0.035,0.06]` |
| `render.width`, `height` | 640×360; integers 1..8192, at most 32 million pixels |
| `render.output` | `output.ppm`, relative to working directory |
| `materials` | Unique names → material definitions |
| `objects` | Array of primitives/groups |
| `lights` | Array, empty by default |
| `include` | Bonus: scene-relative files loaded before this scene |
| `plugins` | Bonus: scene-relative `.so` files loaded before materials/objects |

Assets are relative to their referencing file; output is relative to the working directory. The output directory must exist.

## Bonus rendering

| Field | Default when `render` exists | Range |
|---|---|---|
| `samples` | 16 | 1..4096 |
| `depth` | 6 | 1..24 |
| `threads` | 0 | 0..256; 0 = available CPUs |
| `ao` | 0 | 0..128 |
| `exposure` | 1 | 0.01..100 |
| `toneMap` | `reinhard` | `reinhard`, `neutral`, `none` |
| `colorSpace` | `srgb` in pbr, otherwise `gamma2` | Encoding after averaging/exposure/tone mapping |
| `integrator` | `whitted` | `whitted`, `path`, `photon`, `pbr` |
| `photons` | 0 | 0..2,000,000 |
| `photonRadius` | 0.3 | 0.001..10 |
| `adaptive` | 0 | 0..1; 0 disables adaptive stopping |
| `minSamples` | 4 | 2..4096; evaluate stopping in blocks of four |

Without `render`: 640×360, one sample/thread, depth 6. Adaptive AA never exceeds `samples`, even if `minSamples` is higher. Photon rendering needs a positive budget and point/area source. [Integrator selection](optics.md).

## Primitives

| `type` | Parameters |
|---|---|
| `sphere` | `radius` default 1, centered at origin |
| `plane` | `normal` default `[0,1,0]`, infinite plane through origin |
| `cylinder` | `radius` default 1, Y axis, infinite |
| `cone` | `radius` default 1, infinite double cone `x²+z²=radius² y²` |
| `limited_cylinder` | `radius` default 1, `height` default 2, Y=0..height, two caps |
| `limited_cone` | Base `radius`, `height`, closed base Y=0, tip Y=height |
| `triangle` | Noncollinear `a`, `b`, `c`, visible from both sides |
| `box` | Required positive `size`, centered at origin |
| `torus` | Y axis, `major` default 1.3 (0.1..10), `minor` default 0.35 (0.01..2) |
| `tanglecube` | Fixed surface `Σ(x⁴−5x²)+11.8=0` |
| `fractal` | Power-8 Mandelbulb, `iterations` default 12 (2..24) |
| `mobius` | `radius` default 1.5 (0.1..10), half-`width` default 0.5 (0.01..2), `segments` default 128 (16..1024) |
| `obj` | Scene-relative `file`, usual transformations |

Primitives require a material; groups do not. Usual radii/heights: 10⁻⁶..10⁶; box dimensions positive. Torus/tanglecube use quartics; fractal uses an estimator; Möbius is an open mesh without a volumetric interior.

## Transformations

| Field | Range / default |
|---|---|
| `translate`, `rotate` | `[0,0,0]`; rotate X, then Y, then Z |
| `scale` | `[1,1,1]`, signed factors, absolute value 0.0001..10000 |
| `shear` | `[xy,xz,yz]`, upper-triangular matrix |
| `matrix` | 16 row-major values, invertible affine, final row `[0,0,0,1]` |
| `group` | `children`, parent transformations applied to descendants |

Order: scale/shear → user matrix → rotations → translation. Normals use inverse transpose; AABBs use all eight corners. [Scenegraph example](../bonus/scenes/scenegraph.json).

## Lights

| Type | Fields / ranges |
|---|---|
| All | White `color`, `intensity` |
| `ambient` | Intensity 0..100, default 0.15 |
| `directional` | **Propagation** `direction`; `[0,-1,0]` lights from above; intensity 1, 0..100 |
| `point` | `position`; intensity 1, 0..10000; `falloff` 0..100, default 0 |
| `area` | `position`, full sides `u`,`v`; intensity 1, 0..10000; area 10⁻⁸..10¹²; `twoSided=false` |

Point `physical=true` uses inverse-square and ignores falloff; automatic in path/photon when omitted. Otherwise `1/(1+falloff*d²)`. Area emission faces `normalize(u×v)`, with soft shadows and an opaque back.

```json
{"type":"area","position":[0,5,0],"u":[2,0,0],"v":[0,0,2],"intensity":8}
```

## Materials

`flat` (default) accepts `color`, default `[0.7,0.7,0.7]`. `phong` adds:

| Field | Default | Meaning |
|---|---|---|
| `color` | `[0.7,0.7,0.7]` | Albedo |
| `specular` | 0.25 | Phong highlight strength 0..1 |
| `shininess` | 64 | Exponent 1..2048 |
| `reflection` | 0 | Reflected share 0..1 |
| `transmission` | 0 | Transmitted share 0..1; sum with reflection ≤1 |
| `ior` | 1.5 | Index 1..3 |
| `metallic` | 0 | Albedo-tinted reflection, 0..1 |
| `emission` | `[0,0,0]` | Components 0..100 |
| `pattern` | `solid` | `solid`, `checker`, `marble`, `perlin`, `image` |
| `second` | `[0.1,0.1,0.1]` | Second procedural color |
| `scale` | 1 | Frequency 0.0001..1000 |
| `file` | — | Required for `image` pattern |
| `normalFile` | — | Optional tangent-space normal map |
| `normalStrength` | 1 | Tangential amplitude 0..10 |

`pbr` supports textures, `roughness` 0..1, `absorption` RGB 0..1000. Metal/transmission/IOR use physical lobes with `integrator: pbr`. [Model/settings](pbr-studio.md).

| Texture / format | Support / limits |
|---|---|
| Procedural | Volumetric checker, gradient Perlin, marble |
| Images | P3/P6 8/16-bit, BMP RGB 24/32-bit, raw/RLE TGA 24/32-bit, raw/RLE Radiance RGBE `-Y +X` |
| Dimensions | 1..8192, ≤32 million pixels, file ≤128 MiB |
| Colors | Decode color images to linear; HDR/normal maps already linear; ignore alpha |
| Filtering | Physical mode mipmaps/bilinear/trilinear, cone-footprint approximation |
| HDR | `environment`: `file`, `intensity` 0..10000, `rotation`; linear `.pfm` output |

## OBJ / MTL

| Element | Support |
|---|---|
| OBJ | `v/vt/vn/f/mtllib/usemtl`, positive/negative indices, barycentric UVs/normals |
| Faces | Simple planar concave polygons via ear clipping; ≤4096 vertices/face, ≤1 million vertices/triangles |
| MTL | `newmtl`, `Kd/Ke/Ns/Ni/d/Tr`, `map_Kd/map_Bump/bump/norm`; MTL-relative paths |
| Unsupported | Holes, self-intersections, advanced `map_*` options, projective surfaces; `o/g/s` ignored |

Without normals: flat shading. Without UVs: local X/Z projection. OBJ textures use a bottom-left origin.

## Imports and plugins

Imports load before the current file; objects/lights append, material names are unique, current camera/render settings take precedence. Limits: 16 MiB scene, import depth 32, cycles rejected.

[ABI 3 plugins](architecture.md) · [Worker-root asset checks](network-protocol.md). Outputs use temporary files and rename, preserving the previous image on failure.
