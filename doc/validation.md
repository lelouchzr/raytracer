# Testing

Strict builds use `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`.

| Area | Coverage |
|---|---|
| Geometry | Intersections, transforms, BVH comparisons, implicit surfaces and OBJ imports |
| Rendering | Fresnel/TIR, BSDF energy and PDFs, MIS, media, color and deterministic sampling |
| Studio | Numeric input, serialization, save/reopen, cancellation and native windows |
| Integration | CLI errors, codecs, resource relocation, plugins and TCP workers |
| Gallery | Scene loading, closed sculpture topology and identical PNG/PPM pixels |

```sh
make test
make BUILD=build-core BONUS=OFF test
make format-check
cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug -DRAYTRACER_SANITIZE=ON
cmake --build build-sanitize --parallel 4
ctest --test-dir build-sanitize --output-on-failure
```

On Linux, installing Valgrind enables the CMake memory checks. Window tests need a display or Xvfb; network tests need loopback sockets. Sanitizer leak checks depend on platform support. Graphical libraries may retain process-wide caches; lost blocks and invalid accesses remain errors.

Reports, profiler captures and local machine details are not published. [Scene format](scene-format.md) · [Optical models](optics.md) · [Profiling](performance.md).
