# Subject and project defense

An Epitech project completed during my **second year at Epitech**. Reference: Raytracer v1.4. The engine uses JSON; bonus builds also accept the original format and libconfig example.

| Requirement | Implementation |
|---|---|
| C++ / allowed libraries | C++20, standard library, libdl; SFML viewer; system sockets isolated in network bonus |
| Root `raytracer` executable | CMake staging, supplied Makefile |
| Build targets | `all`, `re`, `clean`, `fclean`, `bonus`, `test` |
| stderr errors, exit 84 | Main exception boundary, CLI tests |
| Sphere/plane/translation/light/camera | `scenes/` demos |
| PPM without mandatory GUI | Headless mode, atomic P6 |
| Primitive/light interfaces | IPrimitive/ILight plus material/integrator contracts |
| At least two design patterns | Factory, Builder, Decorator, Composite groups/BVH |
| Recommended cylinder/cone/rotation/shadows | Included in independent core |
| Bonuses under bonus/ | Implementations, headers, example plugin, scenes/assets |

`RAYTRACER_BONUS=OFF` builds the core without bonus sources. [Features/scenes/tests](feature-matrix.md).

## Short demonstration

| Step | Show / explain |
|---|---|
| Core | `./raytracer --help`, `scenes/`, rays/discriminant |
| Geometry | `torus`, `fractal`, `mobius`, `scenegraph`: exact, approximate, meshed, transformed |
| Lighting | `subject_reference`, `soft_shadows`, `global_illumination`, `caustics` |
| Studio | `./raytracer --studio`: material/camera/light, preview/export |
| Physics | GGX, MIS, absorption, PBRT reference comparison |
| Extensibility | ABI 3 plugin, two TCP workers |
| Validation | `make test`, [reports](validation.md), measurements |

Use bundled screenshots during long calculations. Explain Monte Carlo noise, finite depth/epsilon, fixed-radius photon mapping, closed nested volumes, and replicated network assets. [Optical details](optics.md).
