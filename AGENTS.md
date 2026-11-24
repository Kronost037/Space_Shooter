# Space Shooter · Sector Siege

## Handoff

This is Kronost037/Space_Shooter, a C17/Raylib top-down arcade shooter. Raad maintains the game and has tested it on Arch Linux. The current goal is a readable, satisfying solo campaign that can later become a competitive multiplayer mode. Start from the current GitHub default branch and check open PRs before making assumptions about which work shipped. Avoid taking older local branches or build artifacts as the source of truth.

- PR #11 introduced Sector Siege: separate movement and aim, sector art, Wardens and the Carrier, telegraphs, radar, and a rechargeable pulse.
- PR #12 added the Aster convoy campaign, four difficulty profiles, timer/audio settings, window-resolution UI, per-world seeds, and real-window flight checks.
- PR #13 simplified the menus and moved all UI text to Atkinson Hyperlegible.
- This change adds turn anticipation and settle animation, patrolling Wardens, a third survey beacon, a two-leg cargo escort, and four short cutscenes. Check its PR status before describing it as shipped.

## Design and story

Fly escort for the Aster convoy. The convoy has one remaining jump, the Wardens jam the corridor, and a Command Carrier occupies the gate. The player locates the Wardens, clears the jamming ships, escorts the medical ship Mercy Six through a cargo stop, then clears the jump gate.

The main menu should feel like an arcade title screen. Keep story in animated scenes and short radio lines during play. A menu interaction should be obvious from its label. Use one font family, high-contrast numerals and text at the minimum window size, and reserve amber for warnings and important actions. A single short objective and, when needed, one radio line are enough for the flight HUD. New spectacle should keep hostile telegraphs and the ship visible.

Expand levels when the activity changes. The current beacons teach motion, the Warden patterns teach dodging, the escort makes local control and pulse timing matter, and the Carrier tests those skills. Ideas worth prototyping are alternate beacon routes, optional cargo runs and patrols whose routes converge. Keep optional activities brief and reward taking a risk or completing an objective. Repeated rings and travel without a decision become padding.

## Code map

| Path | Responsibility |
| --- | --- |
| `src/core.c` | Window, input routing between screens, pause, outcome transitions, score submission |
| `src/game.[ch]` | Flight state, fixed-step loop, movement, projectiles and scouts, asset lifetime |
| `src/flight/encounter.c` | Warden patrols, boss attack phases, telegraph/projectile geometry, boss score events |
| `src/flight/mission.c` | Beacon progress, escort route and cargo stop, stage transitions |
| `src/flight/render.c`, `src/arena.c` | Interpolated world, ship and exhaust art, attack graphics, parallax and landmarks |
| `src/ui/display.c`, `src/ui/widgets.c`, `src/ui/cinematic.c` | Viewport/input coordinate contract, shared typography and controls, scripted presentation |
| `src/panel.c` | Combat text, objective markers, boss health, radar, pulse and optional clock |
| `src/rules.c`, `src/settings.c` | Difficulty profiles, local preferences, audio levels |
| `src/leaderboard.c` | Existing local score format and ranking |
| `build.c`, `tests/`, `.github/workflows/c.yml` | Build, regressions, native-window inputs and screenshot artifacts |

## Contracts

- Arrow keys move, WASD aims and fires independently, Space pulses, M toggles radar, Esc pauses, F11 switches borderless fullscreen. Enter or Esc skips a cinematic. The skip frame consumes input. Short scenes are presentation-only and freeze active play, including a configured timer.
- The fixed-step simulation runs at 120 Hz. World-space projectile tests use current and previous positions, independently of UI rendering. A boss must be visible to start attacks. Player shots survive camera motion when fired at a visible target. Damage rules still apply to locked bosses and recovery hits. Keep telegraphs tied to the attack origin and pattern; Wardens hold still during committed patterns.
- The pilot turns visually using interpolated angles and counter-sway. The input response, hitbox and gun aim remain independent of the hull animation. Wardens roam their regions and are tracked by radar, while the Carrier belongs at a fixed landmark.
- Three beacons unlock three Wardens. Clearing every Warden unlocks the escort. Mercy Six waits if the player falls behind or scouts block it. The first stop loads the cargo, the second delivers it and unlocks the Carrier. Escort completion and the win each award points once.
- Difficulty changes hull, patrols, projectile speed, warning time and point payouts. Scout scoring is capped per threat stage. The optional solo timer counts active play. Preserve score and settings compatibility under `.game/`.
- The render texture contains the world. Text and UI draw at native window resolution after the world pass. Mouse hitboxes use logical coordinates through the same letterboxed viewport.

## Multiplayer vision and open work

The intended mode starts many players together, each in an independent world. A server should own the round clock, ruleset, seed and final scores. Finish the round when one player is still active or at the hard deadline, then rank by score. Completion versus elimination, simultaneous finishes, disconnects and score ties need explicit rules. The last surviving player and the score leader are distinct results if the rules permit them.

`resetRunSeeded` and `advanceFlight` already expose the initial seed and input-driving seam. The same-seed check demonstrates matching patrol spawns for identical local inputs. Before networking, separate authoritative simulation from Raylib audio/rendering, use an integer tick and common input format, and make score/terminal events reproducible and independently verifiable. Decide how cutscenes should behave under a shared clock. Start with local headless replay and multiple worlds in one process, then test latency and disconnections before scaling lobbies.

Remaining solo work is playtesting on a real display, balancing the longer escort and Carrier across all four difficulties, and measuring steering readability during reversals and busy fights. Current screenshots and scripted checks cover structural regressions but cannot establish whether pacing feels right.

## Build and verify

From the repo root, compile the helper with `gcc build.c -o build`. Use `./build --download raylib` if the local Raylib dependency is missing, then `./build` and `"./Space Shooter"`. Run `./build --checks && sh tests/verify.sh` with Xvfb. With xdotool installed, the checks also drive the real menus. CI attaches full-resolution screenshots. Review default and compact captures for any visual changes and state clearly which interaction was exercised. Keep build output, screenshots and local save files out of commits.
