#!/usr/bin/env bash
set -euo pipefail
platform=${1:?select linux or windows}
raylib=${2:?supply the extracted Raylib 6.0 SDK directory}
version=$(cat VERSION)
out="$PWD/dist"
mkdir -p "$out"
temp=$(mktemp -d)
trap 'rm -rf "$temp"' EXIT
name="Sector-Siege-$version-$platform-x64"
stage="$temp/$name"
mkdir -p "$stage/src" "$stage/licenses"
mapfile -t sources < <(find src -name '*.c' | sort)
case "$platform" in
    linux) binary=SectorSiege; libs=(-lm -lpthread -ldl -lX11) ;;
    windows) binary=SectorSiege.exe; libs=(-static -mwindows -lopengl32 -lgdi32 -lwinmm -lshell32 -luser32) ;;
    *) exit 2 ;;
esac
"${CC:-gcc}" -std=c17 -O2 -Wall -Wextra -Werror -I"$raylib/include" "${sources[@]}" "$raylib/lib/libraylib.a" "${libs[@]}" -o "$stage/$binary"
cp -R src/Assets "$stage/src/"
cp LICENSE README.md ATTRIBUTION.md "$stage/"
cp "$raylib/LICENSE" "$stage/licenses/raylib.txt"
if [[ $platform == windows ]]; then
    (cd "$temp" && python3 -m zipfile -c "$out/$name.zip" "$name")
else
    tar -czf "$out/$name.tar.gz" -C "$temp" "$name"
fi
