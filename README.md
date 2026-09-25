# half-life-alpha-0.52-fix

Unofficial fixes for the **Half-Life Alpha 0.52** (1997): full save and load, map state kept
across level transitions, engine crash fixes, aiming, AI, controls and graphics. The full list is
in [`docs/FIXES.md`](docs/FIXES.md) and the technical details (addresses, disassembly, tests) in
[`docs/NOTES.txt`](docs/NOTES.txt).

**Unofficial project, not affiliated with Valve. It contains no Valve files:** you need your own
copy of the Half-Life Alpha 0.52. Nothing in the alpha is modified on disk; every change is
applied in memory at startup.

## Installation

1. Copy the alpha 0.52 folder wherever you like (the one with `enginegl.exe` and `valve\`).
2. Copy `winmm.dll` into that folder, next to `enginegl.exe`, and `valve\autoexec.cfg` into its
   `valve\` folder (it replaces the one from the CD). Both come in the zip on the
   [Releases](../../releases) page; `autoexec.cfg` is also in this repo.
3. Move away the `Opengl32.dll` that ships with the alpha (rename it, e.g. to
   `Opengl32-3dfx.dll`). It is the 3Dfx MiniGL and makes the game run on Glide emulation, which
   is much slower; without it the game uses your graphics card's OpenGL.
4. Double-click `enginegl.exe`. It starts at the main menu, at your desktop resolution.

To uninstall, delete `winmm.dll`.

### Controls (`valve\autoexec.cfg`)

The CD's `autoexec.cfg` leaves the mouse without mouselook (moving it up/down walks) and the F keys
bound to Valve's test commands. The one in this repo has modern controls:

| Key | Action | Key | Action |
|---|---|---|---|
| W A S D / arrows | move | mouse | aim |
| Left / right button | fire / secondary fire | / | next weapon |
| Space | jump | C, Ctrl | crouch |
| E, B | use | Q | previous weapon |
| 1–3 | crowbar, pistol, MP5 | Tab | scores |
| Shift | run | ` or ~ | console |
| F2 / F3 | save / load menu | F6 / F7 | quicksave / quickload |
| F4 | options | F10 | quit |
| F5 | screenshot | - / + | HUD size |

It runs on every startup, so it overrides key changes made from the options menu: to change a key
permanently, edit the file.

The DLL only acts if it recognises the files: it checks by hash that `enginegl.exe` and
`valve\dlls\hl.dll` are the original 0.52 ones. Otherwise it leaves everything untouched. Two
`enginegl.exe` with the same code are accepted: the one from the alpha CD and a copy going around
with the `LARGE_ADDRESS_AWARE` bit set in its header (only 4 header bytes differ). What it does is
logged to `hlalpha.log`, next to `enginegl.exe`.

## How it works

`enginegl.exe` imports `WINMM.dll`, and Windows looks for it in the game folder first. This
project's `winmm.dll`:

- forwards all its functions to the system `winmm.dll` (all 193, not only the ones the engine
  uses: any DLL loaded later that uses winmm will bind to this one);
- applies the engine's static patches in memory and, as soon as the engine loads it, those of
  `hl.dll` (`src/patches.h`: only the new bytes, none of Valve's);
- hooks engine functions for save and load, map persistence, the default command line, etc.
  (`src/hlalpha.c`).

## Building

With the i686 mingw from [w64devkit](https://github.com/skeeto/w64devkit):

    ./build.sh             # -> build/winmm.dll

Set `W64DEVKIT` to the w64devkit folder if its `bin` is not in your `PATH`.

Tools (Node):

- `tools/gen_winmm.js`: generates `src/winmm.def` and `src/winmm_stubs.c` from the system's
  32-bit `winmm.dll` (`C:\Windows\SysWOW64\winmm.dll`).
- `tools/gen_patches.js`: generates `src/patches.h` by comparing the original binaries with
  patched versions. It was used once to move to memory the patches that used to be applied to
  the files; new ones can be added to `patches.h` by hand.

## Testing

`hlalpha.c` includes a hook for automated tests across map changes: on the Nth entry into a map
in the session, if `valve\zprueba_N.cfg` exists it is executed, and if `valve\zprueba_pos.txt`
(`x y z pitch yaw`) exists the player is placed there. Without those files it does nothing.
