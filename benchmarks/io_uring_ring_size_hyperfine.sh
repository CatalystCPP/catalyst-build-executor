#!/usr/bin/env bash

# Ensure we bail out if a command fails
set -e
echo -e "\n=== Benchmarking Clean Routines ==="
# We run a build in the prepare step so there is actually something to clean
hyperfine --warmup 2 \
    --shell=none \
    --prepare "/home/som/Projects/catalyst/cob/benchmarks/cobr64 -C heavy_repo" "/home/som/Projects/catalyst/cob/benchmarks/cobr64 -C heavy_repo -t clean" \
    --prepare "/home/som/Projects/catalyst/cob/benchmarks/cobr128 -C heavy_repo" "/home/som/Projects/catalyst/cob/benchmarks/cobr128 -C heavy_repo -t clean" \
    --prepare "/home/som/Projects/catalyst/cob/benchmarks/cobr256 -C heavy_repo" "/home/som/Projects/catalyst/cob/benchmarks/cobr256 -C heavy_repo -t clean" \
    --prepare "/home/som/Projects/catalyst/cob/benchmarks/cobr512 -C heavy_repo" "/home/som/Projects/catalyst/cob/benchmarks/cobr512 -C heavy_repo -t clean" \
    --prepare "/home/som/Projects/catalyst/cob/benchmarks/cobr1024 -C heavy_repo" "/home/som/Projects/catalyst/cob/benchmarks/cobr1024 -C heavy_repo -t clean" \
    --prepare "/home/som/Projects/catalyst/cob/benchmarks/cobr2048 -C heavy_repo" "/home/som/Projects/catalyst/cob/benchmarks/cobr2048 -C heavy_repo -t clean" \
