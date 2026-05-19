# Profiling

Measure the same scenes with matching compiler flags, sampling budgets and thread counts. Compare image checksums before interpreting timing changes. Warm up each binary and alternate repeated baseline/candidate runs.

| Optimization | Preserved behavior |
|---|---|
| AABB component loads | Slab divisions, tolerances and hit ordering |
| Material/media fast paths | Fresnel decisions, random sequence and boundary handling |
| Quartic stack storage | Polynomial evaluation, roots, sorting and tolerances |

## Reproduce locally

```sh
git worktree add --detach /tmp/raytracer-before profiling-baseline
cmake -S /tmp/raytracer-before -B /tmp/raytracer-before/build-perf \
  -DCMAKE_BUILD_TYPE=Release '-DCMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG -g' -DBUILD_TESTING=OFF
cmake --build /tmp/raytracer-before/build-perf --target raytracer -j 2
cmake -S . -B build-perf-candidate -DCMAKE_BUILD_TYPE=Release \
  '-DCMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG -g'
cmake --build build-perf-candidate -j 2
python3 tools/benchmark.py --baseline /tmp/raytracer-before/build-perf/bin/raytracer \
  --candidate build-perf-candidate/bin/raytracer --runs 5 --warmup 1 \
  --report /tmp/raytracer-comparison.json
```

Profiling captures and timing reports stay outside the repository. Stop competing renders while measuring; timings vary with the execution environment. The benchmark rejects incompatible builds and changed images. [Tests](validation.md).
