# Space Shooter

Sector Siege is a top-down arcade shooter written in C with Raylib. Clear three
Wardens to reveal the Command Carrier, then destroy it to finish the mission.

## Play

| Key | Action |
| --- | --- |
| Arrow keys | Move |
| WASD | Fire in eight directions. Hold two adjacent keys to shoot diagonally. |
| Space | Discharge the pulse when ready |
| M | Toggle the radar |
| F11 | Toggle borderless fullscreen |
| Esc | Pause or resume a run. Leave the run from the pause screen. |

The hull strafes independently of the aimable gun. The broken orbital ring marks the fan Warden, the moon marks the tracking lance, and the shipyard marks the alternating lanes. A committed projectile stays dangerous while visible.

The pulse clears nearby hostile bolts, destroys nearby scouts, and deals damage
to engaged bosses. It charges over time and faster when you land kills. The
amber lines show where an attack has committed. Fire during the exposed-core
window for double damage. The Carrier blocks ordinary shots outside that window
and answers with a ring burst at half hull.

Scouts award 10 points at the start and five more per cleared Warden. Wardens
award 100, 150, and 200 points in the order you defeat them, repair one hull pip,
and accelerate scout reinforcements. Only the first 12 scouts in each threat stage
award points. The Carrier awards 500 points, plus up to 250 for a quick clear.
Sector Siege
scores have a dedicated local leaderboard at `.game/sector-siege-leaderboard.bin`.

## Build and run

From the repository root with GCC and Raylib installed or available locally:

```sh
gcc build.c -o build
./build
"./Space Shooter"
```

If Raylib is missing, run `./build --download raylib` before building. This step
uses CMake, wget, unzip, and the X11 build dependencies. See `./build --help`
for the debug build and bootstrap options.
