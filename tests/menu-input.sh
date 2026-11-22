#!/bin/sh
# Run in an isolated working directory under Xvfb.
set -eu
binary=$1
"$binary" > game.log 2>&1 &
pid=$!
trap 'kill "$pid" 2>/dev/null || true' EXIT
window=$(xdotool search --sync --onlyvisible --name "Space Shooter" | head -1)
xdotool windowfocus --sync "$window"
sleep 1
# Hold through at least one rendered frame on a software renderer.
press() {
    xdotool keydown "$1"
    sleep 0.16
    xdotool keyup "$1"
    sleep 0.16
}
press Down
press Down
press Return
sleep 0.5
press Right
press Down
press Right
press Down
press Left
press Down
press Left
press Down
press Left
press Down
press Left
press Escape
sleep 0.3
test "$(cat .game/flight-settings.txt)" = "v1 2 1 4 75 60 80"
# Exit through the menu so asset cleanup runs on the real entry point.
press Down
press Return
wait "$pid"
trap - EXIT
echo "PASS: real window keyboard navigation, six settings persisted, clean exit"
