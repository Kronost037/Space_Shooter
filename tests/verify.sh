#!/bin/sh
set -eu
root=$(pwd)
fixture=$(mktemp -d)
trap 'rm -rf "$fixture"' EXIT
ln -s "$root/src" "$fixture/src"
cd "$fixture"
xvfb-run -a -s "-screen 0 1920x1080x24" "$root/flight-check"
mkdir -p "$root/screenshots"
cp screenshots/*.png "$root/screenshots/"
