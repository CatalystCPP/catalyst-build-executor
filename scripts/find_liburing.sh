#!/bin/sh

# Locate liburing, building a pinned release when it is not installed.
# Catalyst parses stdout, so keep diagnostics and build output on stderr.
set -eu

readonly liburing_version=liburing-2.15
readonly liburing_commit=d41bf9220ec39277ff235379e9089d9e0fd6c2a5

pkg_config=${PKG_CONFIG:-pkg-config}

if command -v "$pkg_config" >/dev/null 2>&1 && "$pkg_config" --exists liburing; then
    "$pkg_config" --cflags --libs liburing
    exit 0
fi

multiarch=
if command -v cc >/dev/null 2>&1; then
    multiarch=$(cc -print-multiarch 2>/dev/null || true)
fi

find_liburing() {
    for root in "$@"; do
        include_dir=
        if [ -f "$root/include/liburing.h" ]; then
            include_dir=$root/include
        elif [ -f "$root/src/include/liburing.h" ]; then
            include_dir=$root/src/include
        else
            continue
        fi

        for lib_dir in "$root/lib" "$root/lib64" "$root/src"; do
            if [ -f "$lib_dir/liburing.so" ] || [ -f "$lib_dir/liburing.a" ]; then
                printf '%s\n' "-I$include_dir -L$lib_dir -luring"
                return 0
            fi
        done

        if [ -n "$multiarch" ]; then
            lib_dir=$root/lib/$multiarch
            if [ -f "$lib_dir/liburing.so" ] || [ -f "$lib_dir/liburing.a" ]; then
                printf '%s\n' "-I$include_dir -L$lib_dir -luring"
                return 0
            fi
        fi
    done

    return 1
}

set --
if [ -n "${LIBURING_ROOT:-}" ]; then
    set -- "$@" "$LIBURING_ROOT"
fi
if [ -n "${LIBURING_DIR:-}" ]; then
    set -- "$@" "$LIBURING_DIR"
fi
set -- "$@" /usr/local /usr

if find_liburing "$@"; then
    exit 0
fi

if [ "$(uname -s)" != Linux ]; then
    printf '%s\n' "liburing is only supported on Linux" >&2
    exit 1
fi

cache_home=${XDG_CACHE_HOME:-${HOME:-/tmp}/.cache}
build_root=${LIBURING_BUILD_ROOT:-$cache_home/cob/$liburing_commit}
source_dir=$build_root/source
build_dir=$build_root/build
install_dir=$build_root/install

if find_liburing "$install_dir"; then
    exit 0
fi

printf '%s\n' "liburing not found; building $liburing_version from GitHub" >&2

if [ ! -d "$source_dir/.git" ]; then
    rm -rf "$source_dir" "$build_dir" "$install_dir"
    mkdir -p "$build_root"
    git clone --depth 1 --branch "$liburing_version" \
        https://github.com/axboe/liburing.git "$source_dir" >&2
fi

resolved_commit=$(git -C "$source_dir" rev-parse HEAD)
if [ "$resolved_commit" != "$liburing_commit" ]; then
    printf 'liburing commit mismatch: expected %s, found %s\n' \
        "$liburing_commit" "$resolved_commit" >&2
    exit 1
fi

jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '1\n')
mkdir -p "$build_dir"
(
    cd "$build_dir"
    "$source_dir/configure" --cc=clang --cxx=clang++ \
        --prefix="$install_dir"
    make -j"$jobs" library
    make install
) >&2

if find_liburing "$install_dir"; then
    exit 0
fi

printf '%s\n' "failed to build liburing in $install_dir" >&2
exit 1
