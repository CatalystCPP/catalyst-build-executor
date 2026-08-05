#!/bin/sh

# Locate an installed liburing and emit pkg-config-style compiler/linker flags.
# Catalyst parses stdout, so keep diagnostics on stderr.
set -eu

dep_name=${CATALYST_DEP_NAME:-liburing}
pkg_config=${PKG_CONFIG:-pkg-config}

if command -v "$pkg_config" >/dev/null 2>&1 && "$pkg_config" --exists liburing; then
    "$pkg_config" --cflags --libs liburing
    exit 0
fi

# Fall back to common install locations and to a configured/built liburing tree.
multiarch=
if command -v cc >/dev/null 2>&1; then
    multiarch=$(cc -print-multiarch 2>/dev/null || true)
fi

for root in ${LIBURING_ROOT:-} ${LIBURING_DIR:-} /usr/local /usr; do
    include_dir=
    if [ -f "$root/include/liburing.h" ]; then
        include_dir=$root/include
    elif [ -f "$root/src/include/liburing.h" ]; then
        include_dir=$root/src/include
    else
        continue
    fi

    for lib_dir in "$root/lib" "$root/lib64" \
        ${multiarch:+"$root/lib/$multiarch"} "$root/src"; do
        if [ -f "$lib_dir/liburing.so" ] || [ -f "$lib_dir/liburing.a" ]; then
            printf '%s\n' "-I$include_dir -L$lib_dir -luring"
            exit 0
        fi
    done
done

printf '%s\n' \
    "$dep_name not found; install liburing development files or set LIBURING_ROOT" >&2
exit 1
