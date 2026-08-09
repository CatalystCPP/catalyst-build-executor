# Xerces-C++ clean-build comparison

Inspired by the  Xerces-C++ 3.3.0 workload from the [build2 “Faster Than Ninja” article](https://build2.org/blog/faster-than-ninja.xhtml).

## Results


| Builder | Mean ± σ [s] | Min [s] | Max [s] | Relative |
|:--|--:|--:|--:|--:|
| COB 0.7.0 | 3.892 ± 0.123 | 3.646 | 4.116 | 1.000 |
| Ninja 1.13.2 | 4.563 ± 0.080 | 4.446 | 4.729 | 1.172 |

COB was **1.172× faster**, corresponding to a **14.7% wall-time reduction**.

Individual passes:

| Order | COB mean ± σ [s] | Ninja mean ± σ [s] |
|:--|--:|--:|
| COB, Ninja | 3.926 ± 0.118 | 4.574 ± 0.083 |
| Ninja, COB | 3.857 ± 0.124 | 4.552 ± 0.080 |

## Workload and validation

- Xerces-C++ 3.3.0 official source archive
  (`SHA-256 9555f1d06f82987fbb4658862705515740414fd34b4db6ad2ed76a2dc08d3bde`).
- `CMAKE_CXX_STANDARD` changed from 14 to 17, as in the article.
- Configuration: Debug, shared library, ICU transcoder, network disabled,
  in-memory message loader, standard mutex manager, and `char16_t` XMLCh.
- Compiler: GCC/G++ 15.2.0.
- Build graph: 297 C++ translation units, 2 C translation units, and one shared
  library link.
- Both builders used 32 jobs and the same sources, object paths, compiler flags,
  link inputs, and link options.
- Both generated exactly 299 object files.
- The final libraries produced by COB and Ninja had the identical SHA-256:
  `4299f85296789e9209014e8697035e7580500ac2229ea9cd73551b64c490c506`.

CMake configuration and generation took 13.95 seconds and was excluded from the
clean-build timings, matching the article’s Ninja methodology.

## Commands

```sh
hyperfine --style=basic --warmup 1 --runs 10 \
  --prepare 'cob -C /tmp/cob-xerces-benchmark/build -t clean -s' \
  'cob -C /tmp/cob-xerces-benchmark/build -j 32 -s' \
  --prepare 'ninja -C /tmp/cob-xerces-benchmark/build -t clean' \
  'ninja -C /tmp/cob-xerces-benchmark/build -j32 src/all'
```

A second pass reversed the two command/prepare pairs. Hyperfine 1.19.0 does not
support randomized command ordering.

## Environment

- CPU: Intel Core i9-14900HX, 32 logical CPUs
- RAM: 30 GiB
- Filesystem: ext4
- CPU governor: `performance`
- Turbo boost: disabled (`intel_pstate/no_turbo=1`)
- Kernel: Linux 7.0.0-28-generic x86-64
- COB: 0.7.0, commit `58f8338e668608de890b3c24de97a531ffb631f8`
- Ninja: 1.13.2
- CMake: 4.2.3
- Hyperfine: 1.19.0
- ICU: 78.2

## Comparability note

COB natively injects `-MMD` for compilation, while this CMake-generated Ninja
build injects `-MD`. The former excludes system headers from emitted dependency
files. Object code and the linked library were nevertheless byte-identical.
COB retains its `.o.d` files, whereas Ninja ingests dependency information into
`.ninja_deps` and removes the original depfiles. These are native behavioral
differences rather than harness modifications.
