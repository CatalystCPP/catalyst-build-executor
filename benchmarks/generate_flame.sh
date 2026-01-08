#!/bin/bash
set -e

# 1. Configuration
PERF_DATA="perf.data"
OUTPUT_SVG="flamegraph.svg"
FLAMEGRAPH_DIR="$HOME/.flamegraph-tools"

# 2. Check if perf.data exists
if [ ! -f "$PERF_DATA" ]; then
    echo "Error: $PERF_DATA not found in current directory."
    exit 1
fi

# 3. Ensure we have the FlameGraph tools
if [ ! -d "$FLAMEGRAPH_DIR" ]; then
    echo "FlameGraph tools not found. Cloning to $FLAMEGRAPH_DIR..."
    git clone https://github.com/brendangregg/FlameGraph "$FLAMEGRAPH_DIR"
else
    echo "Found FlameGraph tools in $FLAMEGRAPH_DIR"
fi

# 4. Generate the FlameGraph
# Pipeline:
#   perf script       -> Dump the raw trace
#   stackcollapse     -> Merge identical stacks (text processing)
#   flamegraph.pl     -> Convert merged stacks to SVG
echo "Generating flamegraph..."

perf script -i "$PERF_DATA" | \
    "$FLAMEGRAPH_DIR/stackcollapse-perf.pl" | \
    "$FLAMEGRAPH_DIR/flamegraph.pl" > "$OUTPUT_SVG"

echo "---------------------------------------------"
echo "Success! Flamegraph saved to: $OUTPUT_SVG"
echo "Open this file in a web browser (Chrome/Firefox) to interact with it."
echo "---------------------------------------------"
