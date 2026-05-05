#!/usr/bin/env python3
"""Reproducible renderer timings, alternating A/B runs, checksums and CPU sampling."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import resource
import statistics
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
SCENES = ROOT / "tools" / "benchmark-scenes"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def command_output(command: list[str]) -> str:
    result = subprocess.run(command, text=True, capture_output=True, check=False)
    return (result.stdout or result.stderr).strip()


def build_metadata(binary: Path) -> dict:
    cache = binary.parent.parent / "CMakeCache.txt"
    flags = {}
    if cache.exists():
        for line in cache.read_text().splitlines():
            if "=" in line and line.startswith(("CMAKE_BUILD_TYPE:", "CMAKE_CXX_FLAGS:",
                                                 "CMAKE_CXX_FLAGS_RELEASE:",
                                                 "CMAKE_CXX_COMPILER:", "RAYTRACER_BONUS:",
                                                 "RAYTRACER_WERROR:", "RAYTRACER_SANITIZE:")):
                name, value = line.split("=", 1)
                flags[name.split(":", 1)[0]] = value
    compiler = flags.get("CMAKE_CXX_COMPILER", "c++")
    stamp = binary.parent.parent / "performance-source.json"
    return {"path": str(binary), "sha256": sha256(binary), "cmake": flags,
            "compiler_version": command_output([compiler, "--version"]),
            "sources": json.loads(stamp.read_text()) if stamp.exists() else None}


def prepare(case: Path, work: Path) -> tuple[Path, Path, dict]:
    scene = json.loads(case.read_text())
    settings = scene["render"]
    if settings.get("threads") != 1 or settings.get("adaptive", 0) != 0:
        raise ValueError(f"{case}: benchmark requires one thread and fixed sample count")
    work.mkdir(parents=True, exist_ok=True)
    output = work / f"{case.stem}.ppm"
    scene["render"]["output"] = str(output)
    config = work / case.name
    config.write_text(json.dumps(scene, indent=2) + "\n")
    return config, output, {"source": str(case.relative_to(ROOT)),
                            "source_sha256": sha256(case),
                            "render": {k: v for k, v in settings.items() if k != "output"},
                            "objects": len(scene["objects"])}


def invoke(binary: Path, config: Path, output: Path) -> dict:
    output.unlink(missing_ok=True)  # Never accept a stale output from a previous invocation.
    before = resource.getrusage(resource.RUSAGE_CHILDREN)
    start = time.perf_counter_ns()
    result = subprocess.run([str(binary), str(config)], cwd=ROOT, capture_output=True,
                            text=True, timeout=180)
    elapsed = (time.perf_counter_ns() - start) / 1e9
    after = resource.getrusage(resource.RUSAGE_CHILDREN)
    if result.returncode:
        raise RuntimeError(f"{binary} exited {result.returncode}: {result.stderr}")
    if not output.is_file():
        raise RuntimeError(f"renderer did not create {output}")
    return {"wall_s": elapsed,
            "user_s": after.ru_utime - before.ru_utime,
            "system_s": after.ru_stime - before.ru_stime,
            "ppm_sha256": sha256(output), "ppm_bytes": output.stat().st_size,
            "stderr": result.stderr.strip()}


def summarize(runs: list[dict]) -> dict:
    wall = [run["wall_s"] for run in runs]
    return {"median_wall_s": statistics.median(wall), "min_wall_s": min(wall),
            "max_wall_s": max(wall), "median_user_s": statistics.median(
                run["user_s"] for run in runs), "runs": runs}


def timed(args: argparse.Namespace, report: dict) -> None:
    binaries = {"baseline": args.baseline.resolve()}
    if args.candidate:
        binaries["candidate"] = args.candidate.resolve()
    report["binaries"] = {name: build_metadata(path) for name, path in binaries.items()}
    if len(binaries) == 2:
        flags = [metadata["cmake"] for metadata in report["binaries"].values()]
        required = {"CMAKE_BUILD_TYPE", "CMAKE_CXX_COMPILER", "CMAKE_CXX_FLAGS",
                    "CMAKE_CXX_FLAGS_RELEASE", "RAYTRACER_BONUS", "RAYTRACER_WERROR",
                    "RAYTRACER_SANITIZE"}
        if any(required - set(values) for values in flags):
            raise ValueError("comparison requires complete CMake flags for both builds")
        if flags[0] != flags[1]:
            raise ValueError(f"baseline and candidate build flags differ: {flags}")
    report["cases"] = {}
    for case in args.cases:
        config, output, metadata = prepare(SCENES / f"{case}.json", args.work.resolve())
        runs = {name: [] for name in binaries}
        checksums = set()
        for _ in range(args.warmup):
            for binary in binaries.values():
                checksums.add(invoke(binary, config, output)["ppm_sha256"])
        for index in range(args.runs):
            order = list(binaries)
            if index % 2:
                order.reverse()  # Alternate AB/BA to reduce ordering and thermal bias.
            for name in order:
                measured = invoke(binaries[name], config, output)
                checksums.add(measured["ppm_sha256"])
                measured["round"] = index + 1
                runs[name].append(measured)
                print(f"{case} {name} {index + 1}/{args.runs}: "
                      f"{measured['wall_s']:.6f}s", flush=True)
        if len(checksums) != 1:
            raise RuntimeError(f"{case}: PPM output changed across binaries/runs: {checksums}")
        result = {**metadata, "ppm_sha256": next(iter(checksums)),
                  **{name: summarize(values) for name, values in runs.items()}}
        if "candidate" in runs:
            result["speedup"] = (result["baseline"]["median_wall_s"] /
                                 result["candidate"]["median_wall_s"])
        report["cases"][case] = result
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n")


def profile(args: argparse.Namespace, report: dict) -> None:
    if sys.platform != "darwin":
        raise RuntimeError("CPU sampling mode currently uses macOS /usr/bin/sample")
    binary = args.baseline.resolve()
    config, output, metadata = prepare(SCENES / f"{args.profile}.json", args.work.resolve())
    invoke(binary, config, output)  # Untimed cache warmup; sampler run is excluded from timing.
    args.report.parent.mkdir(parents=True, exist_ok=True)
    sample_path = args.report.with_suffix(".sample.txt").resolve()
    output.unlink(missing_ok=True)
    start = time.perf_counter()
    process = subprocess.Popen([str(binary), str(config)], cwd=ROOT,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        sampler = subprocess.run(["/usr/bin/sample", str(process.pid),
                                  str(args.sample_seconds), "1", "-mayDie", "-fullPaths",
                                  "-file", str(sample_path)],
                                 capture_output=True, text=True, timeout=args.sample_seconds + 15)
        stdout, stderr = process.communicate(timeout=180)
        if process.returncode:
            raise RuntimeError(f"profiled renderer failed: {stderr}")
        if sampler.returncode or not sample_path.is_file():
            raise RuntimeError(f"CPU sampler failed: {sampler.stdout}\n{sampler.stderr}")
        sample_text = sample_path.read_text()
        if "Thread_" not in sample_text or "rt::" not in sample_text:
            raise RuntimeError(f"CPU sample contains no renderer call stacks: {sample_path}")
        report.update({"binary": build_metadata(binary), "case": metadata,
                       "profile_wall_s": time.perf_counter() - start,
                       "ppm_sha256": sha256(output), "sample": str(sample_path),
                       "sampler_stdout": sampler.stdout, "sampler_stderr": sampler.stderr,
                       "renderer_stderr": stderr, "renderer_stdout": stdout})
        args.report.write_text(json.dumps(report, indent=2) + "\n")
        print(sample_path)
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path, required=True)
    parser.add_argument("--candidate", type=Path)
    parser.add_argument("--stamp-build", action="store_true")
    parser.add_argument("--runs", type=int, default=5)
    parser.add_argument("--warmup", type=int, default=1)
    parser.add_argument("--cases", nargs="+", default=["bvh_1025", "gallery", "path_gi", "quartic"])
    parser.add_argument("--profile", choices=["bvh_1025", "gallery", "path_gi", "quartic"])
    parser.add_argument("--sample-seconds", type=int, default=2)
    parser.add_argument("--work", type=Path, default=ROOT / "build-benchmark-work")
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    if args.runs < 5 or args.warmup < 1:
        parser.error("at least five timed runs and one warmup are required")
    if args.sample_seconds < 1:
        parser.error("sample duration must be at least one second")
    report = {"host": platform.platform(), "machine": platform.machine(),
              "logical_cpus": os.cpu_count(), "python": sys.version,
              "git_head": command_output(["git", "rev-parse", "HEAD"]),
              "warmup_per_binary": args.warmup, "timed_runs_per_binary": args.runs,
              "method": "sampled CPU run excluded; wall timings without instrumentation; AB/BA"}
    if args.stamp_build:
        cache = args.baseline.resolve().parent.parent / "CMakeCache.txt"
        source_root = None
        for line in cache.read_text().splitlines():
            if line.startswith("CMAKE_HOME_DIRECTORY:"):
                source_root = Path(line.split("=", 1)[1]).resolve(strict=True)
        if source_root is None:
            raise RuntimeError("cannot identify CMake source directory for source stamp")
        source_hash = hashlib.sha256()
        paths = [source_root / "CMakeLists.txt"]
        for directory in ("include", "src", "bonus/include", "bonus/src", "bonus/plugins"):
            paths += [p for p in (source_root / directory).rglob("*")
                      if p.suffix in (".hpp", ".cpp")]
        for path in sorted(paths):
            source_hash.update(str(path.relative_to(source_root)).encode())
            source_hash.update(b"\0")
            source_hash.update(path.read_bytes())
        report["source_directory"] = str(source_root)
        report["source_git_head"] = command_output(["git", "-C", str(source_root),
                                                     "rev-parse", "HEAD"])
        stamp = args.baseline.resolve().parent.parent / "performance-source.json"
        if stamp.exists():
            raise RuntimeError(f"refusing to overwrite existing source stamp {stamp}")
        report["production_source_sha256"] = source_hash.hexdigest()
        report["binary_sha256"] = sha256(args.baseline.resolve())
        encoded = json.dumps(report, indent=2) + "\n"
        stamp.write_text(encoded)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        if args.report.resolve() != stamp:
            args.report.write_text(encoded)
        print(stamp)
    elif args.profile:
        profile(args, report)
    else:
        timed(args, report)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"benchmark: {error}", file=sys.stderr)
        sys.exit(1)
