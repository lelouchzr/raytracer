# Features and evidence

Core scenes: `scenes/`. Extensions: `bonus/scenes/`.

| Feature | Method / main source | Scene | Test |
|---|---|---|---|
| Sphere, plane | Analytic intersections, `src/geometry/` | `demo_sphere`, `demo_plane` | `raytracer_unit`: tangent/interior/parallel |
| Infinite cylinder/cone | Quadrics, including linear degeneracy | `demo_cylinder`, `demo_cone` | `raytracer_unit` |
| Translation/rotation | Decorator, `src/transform/` | Core demos | `raytracer_unit` |
| Lights/shadows | ILight, bounded visibility | All scenes | CLI light variations |
| Closed finite shapes | Truncated quadrics and caps | `geometry` | Cap tests |
| Torus | Quartic roots isolated by derivatives | `torus` | `extended_features`: outside/inside; independent tangency probes |
| Tanglecube | Exact ray quartic | `tanglecube` | Hit/finite normal |
| Fractal | Power-8 Mandelbulb distance estimator | `fractal` | `extended_features`, CLI render |
| Möbius | Triangulated parametric surface with half twist | `mobius` | Hit and CLI |
| Triangle/box | Möller–Trumbore / slabs | `geometry` | Units/render |
| Advanced OBJ | `v/vt/vn`, barycentric interpolation, MTL, ear clipping | `obj`, concave fixture | `systems_features`: concave/MTL/UV/invalid |
| Matrix/shear/signed scale | Mat4, Gauss-Jordan inverse, inverse-transpose normals | `scenegraph` | Round trip/shear/mirror |
| Scene graph | Composite groups, nested transforms | `scenegraph` | Real hierarchy intersection |
| Point/color lights | Artistic or inverse-square attenuation | `gallery`, `caustics` | Inverse-square ratio |
| Area lights/soft shadows | Finite parallelogram, area/solid-angle conversion, NEE without double counting | `soft_shadows`, `global_illumination` | `lighting_quality`: quadrature/variance/penumbra/back/mirror/photon flux |
| Exposure/tone mapping | Linear light, optional Reinhard, gamma after averaging | `subject_reference` | Numeric checks, invalid CLI key |
| Phong/reflection/transparency | Recursive Whitted integrator | `gallery`, `nested_media` | Deterministic CLI |
| Nested refraction | Boundary stack, Snell/Schlick/TIR | `nested_media` | Air/glass/water, interior camera |
| Global illumination | Cosine diffuse paths, roulette, emission | `global_illumination` | Indirectly lit floor, black Whitted control |
| Caustics | Light→specular chain→diffuse photons, spatial grid/kernel | `caustics` | No deposit without specular; floating partial glass, correct floor receiver |
| Checker/Perlin/marble | Gradient Improved Perlin, fixed permutation | `normal_perlin`, `texture` | Integer points, periodicity, continuity |
| Normal mapping | Tangent-space map, TBN, separate geometric normals | `normal_perlin` | CLI and preserved boundary normals |
| Image textures | P3/P6 8/16-bit, RGB BMP, raw/RLE TGA | `texture`, runtime fixtures | Equivalent images, truncation |
| Imports | Relative paths, cycle/depth limits | Runtime fixtures | Valid/cyclic CLI imports |
| Supersampling/adaptive AA | Stratification, Welford variance/standard error | `adaptive` | Constant pixel stops at 4 samples |
| BVH | Median-split Composite | `bvh_garden`, 361 spheres | 1,200 rays versus linear search |
| Multithreading | Independent rows, pixel seeds | All scenes | Identical 1/4-thread images |
| Progressive GUI | Synchronized per-row snapshots | `--gui gallery.json` | Real SFML window, `tests/gui.py` |
| Cancel/preview/reload | RenderControl, settings before photons, join guard | `--gui --watch`, `--preview` | Invalid→valid reload, cancel 2M photons |
| Physical materials | GGX BSDF, visible normals, reflection/transmission PDFs, Fresnel | `pbr_studio`, `pbr_showcase` | `pbr_physics`: furnace/PDF/reciprocity/TIR/equal indices |
| MIS/absorption | Power heuristic, RGB Beer–Lambert stack | `pbr_showcase` | MIS vs BSDF, independent PBRT |
| HDR environment | Raw/RLE RGBE, luminance/solid-angle CDF | `pbr_studio` | Uniform source, normalized PDF, invalid RLE |
| Filtering/color/HDR | Mipmaps, cone footprint, integrated checker, sRGB, linear PFM | `pbr_showcase` | HDR >1, identical cluster |
| Native C++ studio | SDL2/ImGui, inspectors, browser, progressive worker | `--studio` | `native_studio_model`, real-window PNG CRC |
| Dynamic plugins | libdl wrapper, ABI 3, leases | `plugin` | Primitive/material/light outlive Factory, bad ABI rejected |
| Distributed rendering | TCP RTR1, 16-row bands, network doubles | `--cluster ... gallery.json` | Two real workers, identical image, protocol/path errors |
| Legacy/libconfig formats | Parsing/normalization, no external dependency | `tests/legacy/` | Six historical scenes rendered, subject syntax, strict errors |
| Errors/atomic output | Entry-point exceptions, temporary file/rename | Invalid configurations | Exit 84, stderr, no partial file |

[Optical models/limits](optics.md) · [Scene format](scene-format.md) · [Current qualification](validation.md).
