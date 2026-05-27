# Render quality — October 29, 2026 audit

Historical report from `35988e8`, covering classic-renderer fixes. [Current PBR](pbr-studio.md) subsequently added GGX, MIS, filtering, HDR, and **ABI 3**.

## Findings and fixes

| Observation | Explanation / action |
|---|---|
| Apparently faceted shapes | Checker/wall reflections, magnified refracted objects; inspect normals and render without mirrors |
| Silhouettes | Analytic checks over 128 sphere/cylinder/cone directions, camera FOV/aspect |
| Compressed colors | Add `toneMap: none` for classic scenes; retain Reinhard default |
| Overly hard shadows | Finite area light and next-event estimation |
| Grazing-angle acne | Offset to the correct side of geometric normal; fixed epsilon |

[Previous gallery](../bonus/quality/before-gallery.png) · [Previous GI](../bonus/quality/before-global_illumination.png) · [Current gallery](../gallery/05_primitive_museum.png). Scenes changed too, so this visual comparison does not isolate the algorithm.

## Verified formulas

| Calculation | Formula / convention |
|---|---|
| Ray | `P(t)=O+tD` |
| Sphere | `a=D·D`, `b=(O−C)·D`, `c=(O−C)·(O−C)−r²`; solve `at²+2bt+c=0` |
| Normal | `(P−C)/r`; inverse transpose after transformation |
| Lambert / Phong | `albedo*max(0,N·L)` / `ks*max(0,R·V)^shininess` |
| Area source | `A=norm(u×v)`, normal `normalize(u×v)` |
| Solid-angle PDF | `pω=d²/(A*cosLight)`; contribution `Le/pω` |
| Diffuse path | Multiply by `albedo/π`, receiving cosine, visibility |
| Photon flux | `π*A*Le` per face |

Local directions are not renormalized, preserving `t`. `reflection` controls a perfect mirror rather than roughness.

Sources: [RTIOW](https://raytracing.github.io/books/RayTracingInOneWeekend.html), [Gambetta: intersections](https://www.gabrielgambetta.com/computer-graphics-from-scratch/02-basic-raytracing.html), [lighting](https://www.gabrielgambetta.com/computer-graphics-from-scratch/03-light.html), [PBRT: area lights](https://www.pbr-book.org/4ed/Light_Sources/Area_Lights), [rounding error](https://www.pbr-book.org/4ed/Shapes/Managing_Rounding_Error).

## NEE / BSDF reference

Same 2×2 square at height 2, albedo 0.6, radiance 3; 40,000 fixed-seed estimates.

| Linear measurement | Value |
|---|---:|
| Reference integral, 400×400 midpoint quadrature | 0.431022 |
| Explicit-sampling mean | 0.431304 |
| BSDF-only continuation mean | 0.428805 |
| BSDF variance / explicit-sampling variance | 103.6 |

This variance ratio is scene-specific, not a universal speedup. Classic path avoids double counting without MIS; ordinary emissive materials use continuation. `pbr` uses MIS.

[BSDF](../bonus/quality/bsdf.png) · [NEE](../bonus/quality/nee.png): same geometry, lighting, budget, 640×400 / 64 spp. NEE adds shadow rays.

## Reproduce

```sh
make
./raytracer bonus/scenes/subject_reference.json
./raytracer bonus/scenes/soft_shadows.json
./raytracer bonus/quality/bsdf.json
./raytracer bonus/quality/nee.json
build/lighting_quality_tests build/plugins/raytracer_example.so
python3 bonus/render_gallery.py
```

| Image | Budget for this release |
|---|---|
| `subject_reference` | 1280×800, 64 spp, finite shapes without mirrors |
| `soft_shadows` | 1280×800, 256 spp |
| `gallery` | 1280×800, 384 spp |
| `global_illumination` | 1024×768, 1024 spp, depth 8 |

PPM contains actual pixels; PNG conversion has no retouching. 25-pair manifest. Generation times include concurrent builds and are not benchmarks.

Core demos intentionally show infinite cylinders/cones. Finite depth, fixed epsilon, indirect noise remain. [Current models/limits](optics.md).
