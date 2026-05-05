#!/usr/bin/env python3
"""Extract renderer leaf samples without summing overlapping recursive stacks."""
import argparse
import json
from pathlib import Path
import re


def summarize(path):
    text = path.read_text()
    graph = text.split("Call graph:\n", 1)[1].split("\nTotal number", 1)[0]
    threads = list(re.finditer(r"^\s+(\d+) Thread_[^\n]*$", graph, re.MULTILINE))
    workers = []
    for index, match in enumerate(threads):
        end = threads[index + 1].start() if index + 1 < len(threads) else len(graph)
        if "rt::RowRenderer::row" in graph[match.end():end]:
            workers.append(int(match.group(1)))
    if len(workers) != 1:
        raise ValueError(f"{path}: expected one sampled renderer worker, got {workers}")
    footer = text.split("Sort by top of stack, same collapsed (when >= 5):\n", 1)[1]
    footer = footer.split("\nBinary Images:", 1)[0]
    renderer, allocator = {}, {}
    for line in footer.splitlines():
        match = re.match(r"\s+(.+?)  \(in (.+?)\)\s+(\d+)\s*$", line)
        if not match:
            continue
        symbol, binary, samples = match.groups()
        if binary == "raytracer":
            renderer[symbol] = int(samples)
        if "malloc" in binary:
            allocator[symbol] = int(samples)
    return {"sample_file": str(path), "worker_samples": workers[0],
            "renderer_top_of_stack": renderer, "allocator_top_of_stack": allocator,
            "note": "Exclusive leaf samples from the footer; counts below five are omitted. "
                    "Worker sample fractions are statistical diagnostics, not cycle counts."}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = {path.name: summarize(path) for path in sorted(args.directory.glob("*.sample.txt"))
              if "empty-attempt" not in path.name}
    args.output.write_text(json.dumps(result, indent=2) + "\n")


if __name__ == "__main__":
    main()
