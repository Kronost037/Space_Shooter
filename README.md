# Sector Siege

The hospital ship Mercy Six has one jump left. Its ward is running on reserve, three Wardens are jamming the corridor, and a Command Carrier holds the gate. You fly the escort. Sector Siege is a single-player, keyboard-controlled arcade shooter set in one continuous sector.

## Play

Download the **Windows x64 ZIP** or **Linux x64 tar.gz** from [Releases](https://github.com/Kronost037/sector_siege/releases). Extract the whole archive to a writable folder. On Windows open **SectorSiege.exe**. On Linux run **./SectorSiege** from the extracted folder. The game includes its artwork, audio, fonts and Raylib. Linux needs an X11 or XWayland display and a working OpenGL driver.

Choose **Play**, enter your callsign and launch. **Medium** is the default difficulty. Easy gives you longer attack warnings. The optional timer counts active flight and pauses during radio scenes.

| Control | Action |
| --- | --- |
| Arrow keys | Fly and strafe |
| WASD | Aim and fire independently of movement |
| Space | Discharge a charged pulse |
| M | Toggle sector radar |
| Esc | Pause or resume |
| Enter or Esc during a transmission | Skip the scene |
| F11 | Toggle borderless fullscreen |

## The crossing

1. **Dead Air.** Fly between the three lights at each navigation fix to reveal the blockade.
2. **Breakwater.** Hunt the three Wardens in any order. Watch the amber warning lines. The lance locks before it fires, and exposed cores take double damage.
3. **Mercy Run.** Stay near Mercy Six while she loads medicine. Interceptors can stall the transport. A pulse clears nearby scouts and hostile bolts.
4. **Last Crossing.** Defeat the Carrier at the gate. Below half hull its ring battery fires twice, with a different opening on the second volley.

Beacons, scout bounties, Wardens and delivery earn points. Each defeated Warden repairs one hull pip. A fast clear earns a bonus, and difficulty scales the payouts. The debrief shows your callsign, difficulty and final points. Records and settings are stored in the game's local **.game** folder.

## Build from source

On Linux, install GCC and Raylib 6.0 development files, then run the following from the repository root.

```sh
gcc build.c -o build
./build
"./Space Shooter"
```

For a local Raylib build, run `./build --download raylib` before `./build`. It uses wget, unzip, CMake and X11 development libraries. Run `./build --checks && sh tests/verify.sh` for the flight checks and UI captures under Xvfb. The shell packaging script accepts a platform and an extracted official Raylib SDK. Use a MinGW-w64 compiler for the Windows target.

The source code is covered by the [MIT License](LICENSE). See the [game content terms](GAME_CONTENT_LICENSE.md) for artwork, story, and original audio, and [ATTRIBUTION.md](ATTRIBUTION.md) for third-party credits and notices.
