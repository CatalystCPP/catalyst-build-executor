# CLI Reference

```
Usage: cob [options]

Options:
  -h, --help                    Show this help message
  -v, --version                 Show version
  -C <dir>                      Change working directory before doing anything
  -f <file>                     Use <file> as the build manifest (default: catalyst.build)
  -j, --jobs <N>                Set number of parallel jobs (default: auto)
  -k, --keep-going              Continue the build after error (default: false)
  -n, --dry-run                 Print commands without executing them
  -s, --silent                  Suppress cli output, only print errors
  -t <tool>                     Run a subtool. Valid tools are:
                                  clean    - remove build artifacts
                                             (-i: clean only compiler outputs)
                                  compdb   - generate compile_commands.json
                                  graph    - generate DOT graph of build
                                  commands - print commands that would be executed
```
