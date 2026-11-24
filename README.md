# Space Shooter · Sector Siege

The Aster convoy has one jump left. Fly a four-stage solo campaign across a continuous sector map.

| Stage | Objective |
| --- | --- |
| I · Dead Air | Fly through three navigation beacons to locate the blockade. |
| II · Breakwater | Hunt three patrolling Wardens in any order. The orbital ring teaches fan volleys, the moon teaches lance locks, and the shipyard teaches staggered lanes. |
| III · Mercy Run | Escort the hospital ship to its medicine pickup, then deliver the cargo at the jump gate. Stay within 290 units and clear scouts within 115 units of the transport. Your pulse can clear a stalled route. |
| IV · Last Crossing | Fight the Command Carrier. At half hull its attack sequence changes and the ring battery fires twice with a rotating opening. At most one slow scout escorts it while the ship is in view. |

## Controls

| Input | Action |
| --- | --- |
| Arrow keys | Strafe. Release to brake. |
| WASD | Fire independently in eight directions. |
| Space | Discharge a charged pulse. Clears nearby hostile bolts and scouts, and damages nearby unlocked bosses. |
| M | Toggle radar |
| Esc | Skip a scene or pause and resume flight. |
| Enter | Skip a cutscene. |
| F11 | Toggle borderless fullscreen |

Amber traces warn of incoming fire. The lance stops tracking before its discharge. Crossing batteries fire inward from two muzzles. Ordinary hits damage every unlocked boss, and hits during recovery deal double damage. The pulse recharges in 15 seconds, with charge also awarded for kills.

## Flight settings and scores

| Difficulty | Warden / Carrier hull | Score multiplier |
| --- | --- | --- |
| Easy | 20 / 36 | 0.80× |
| Medium | 26 / 46 | 1.00× |
| Hard | 32 / 56 | 1.25× |
| Insane | 38 / 66 | 1.50× |

Difficulty also changes scout speed, reinforcement cadence, projectile speed and warning duration. The optional 1–60 minute timer measures active play and freezes on pause and during cutscenes. Expiry ends the run with the score already earned. Master, music and effects volume are saved separately in `.game/flight-settings.txt`.

Each beacon awards 15 base points. Wardens pay 100, 150 and 200 and repair one hull pip. Scouts pay 10 plus five per cleared Warden, capped at 12 payouts per threat stage. The escort pays 90. The Carrier pays 500 plus up to 250 for a fast clear. Each payout is multiplied by difficulty and rounded to the nearest point. Flight records use the existing local leaderboard format.

## Build and verify

From the repository root with GCC and Raylib installed:

```sh
gcc build.c -o build
./build
"./Space Shooter"
```

For a local Raylib build, run `./build --download raylib` first. It uses CMake, wget, unzip and the X11 development libraries. Run `./build --checks && sh tests/verify.sh` to exercise the simulation and capture the interface under Xvfb. The checks use temporary flight data.

## Code layout

- `game.c` owns the per-flight state, fixed-step simulation, input sampling and asset lifetime.
- `flight/mission.c` owns campaign objectives. `flight/encounter.c` shares attack geometry between telegraphs and projectiles. `flight/render.c` interpolates the action for display.
- `rules.c` holds difficulty profiles. `ui/` owns viewport composition, typography and widgets. The HUD renders directly into the window after the world pass.
- `resetRunSeeded` starts a world with an explicit spawn seed. `advanceFlight` accepts movement, aim, pulse and elapsed time. These are the entry points for testing and future parallel-world play. Networking, elimination and online score arbitration remain future work.
