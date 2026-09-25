# Half-Life Alpha 0.52 — fixes

A record of everything fixed in the engine (`enginegl.exe`) and in the game DLL (`hl.dll`) of the
1997 alpha. The full technical details (addresses, disassembly, tests) are in
[`NOTES.txt`](NOTES.txt); the matching section is cited as **[N]**.

Everything lives in `winmm.dll`, which the engine loads at startup; nothing in the alpha is
modified on disk. The tag at the end of each fix says where it is:

- **`patches.h`** (`src/patches.h`): bytes written in memory over `enginegl.exe` or `hl.dll`
  (only the new ones; the originals are recognised by their hash).
- **`hlalpha.c`** (`src/hlalpha.c`): engine functions hooked in memory (save and load,
  transitions, default command line...).
- **`valve/autoexec.cfg`**: the controls configuration copied next to the game.

---

## Saving and loading

### Saving crashed the game · [1]
- **Symptom:** the game closed when saving.
- **Cause:** saving allocated a fixed 64 KB buffer and any real map needs more (the stock `.sav`
  files are about 66 KB). It always overflowed and corrupted memory.
- **Fix:** buffer enlarged to 1 MB. *(hlalpha.c)*

### Loading a game did not exist · [2]–[8]
- **Symptom:** the alpha saves in the new Half-Life format, but loading was Quake's text reader,
  which does not understand it; besides, `hl.dll` does not save the entities' internal C++ state
  (their `Save` and `Restore` are empty).
- **Fix:** a complete save of our own: a block (`CLDX` format) with the variables and private
  state of every entity is appended to the `.sav`, and loading is rewritten from scratch.
  *(hlalpha.c)*

### Loading after closing and reopening the game · [13]
- **Symptom:** loading worked within the same session, but crashed after restarting the game.
- **Cause:** the `.sav` stored memory addresses from that process; `hl.dll` is loaded at a
  different address on every start.
- **Fix:** pointers are saved as offsets and relocated on load (`CLDX` v3 format). *(hlalpha.c)*

### Loading from a multiplayer game crashed · [26]
- **Symptom:** with a multiplayer game running, F7 ended in an engine error.
- **Cause:** while a server is running the engine does not allow changing the number of players,
  so the map started for several, with entities numbered differently, and the single-player save
  was dumped on top of it, mismatched.
- **Fix:** loading shuts down the current game first; and if the map still starts for more than
  one player, nothing is restored. *(hlalpha.c)*

### Links between entities broken on load · [8]
- **Cause:** zeroing each entity's header broke the engine's linked list of areas.
- **Fix:** the header is kept when each entity is restored. *(hlalpha.c)*

### Cold loading (right after startup) · [9]
- **Partial fix:** restoring happens when the client finishes connecting (`begin` command), not
  right after `map`. *(hlalpha.c)*

### After loading you always faced the same direction · [10]
- **Cause:** the server restored the angles, but the client kept its own copy.
- **Fix:** after loading, the client's view is forced (`fixangle`). *(hlalpha.c)*

### After loading the view stayed tilted · [11], [21]
- **Symptom:** if you saved while strafing, or stuck in a door that was hurting you, after
  loading the camera started tilted and never straightened.
- **Cause:** when forcing the view, the server sends the client its three angles, and the third
  is the roll of the player *model*, recomputed from the sideways velocity (the door pushes you
  sideways). The client copied it into its view and nothing reset it. Zeroing the roll on load
  ([11]) was not enough: it was recomputed before being sent.
- **Fix:** the client discards the roll it receives when its view is forced; the roll while
  walking is still visible because it is applied separately, at draw time. *(patches.h)*

### Saving took more than a second · [41]
- **Symptom:** every save (and every level change, which saves the map being left) froze the game
  for about 1.3 s.
- **Cause:** a query to the system about memory, which takes ~2.5 ms on this machine, was
  repeated once per entity, almost always for the same address.
- **Fix:** the answer is remembered. A full save drops to about 10 ms. *(hlalpha.c)*

### Unreadable save menu and no quicksave · [17]
- **Symptom:** the load menu (F3) showed every slot as `VALV` and did not include the quicksave.
- **Cause:** the menu reads the `.sav` files as if they were Quake's; and the quicksave was saved
  as `quick.sav`, outside the 12 slots `s0`–`s11`.
- **Fix:** each slot shows map, date and time; the stock saves (1997, without a `CLDX` block) are
  shown as not loadable and cannot be selected; slot 12 is the quicksave (F6/F7 save to and load
  from `s11`). *(hlalpha.c + valve/autoexec.cfg)*

---

## Level changes

### The player arrived standing still after a map change · [27]
- **Symptom:** momentum was lost when going through a transition.
- **Cause:** the alpha stores the player's velocity when crossing the exit, together with position
  and view, but when spawning in the new map it restores everything except the velocity.
- **Fix:** the player appears with the velocity they had. *(patches.h, on hl.dll)*

### The view came out inverted after a map change · [34]
- **Symptom:** if you were looking down when crossing a transition, you appeared looking up (and
  vice versa).
- **Cause:** when spawning in the new map, the game sent the client the pitch of the player model
  (inverted and divided by 3) instead of the view's.
- **Fix:** the view you had is sent. *(patches.h, on hl.dll)*

### Maps keep their state when you come back · [39]
- **Before:** when returning to a map through a transition, buttons, doors and enemies were as at
  the start.
- **Now:** when you leave a map its state is saved, and it is restored when you come back (you
  arrive with your health, weapons and velocity). Saved games include the state of the visited
  maps; a new game starts with every map fresh. *(hlalpha.c)*
- **Correction [40]:** after several round trips it crashed: entities created during the game
  were restored with a null internal pointer. They are now restored correctly, and restored
  entities are relinked into their place in the map.

---

## Crashes and hangs

### Saving after switching weapons crashed the game · [29]
- **Symptom:** "NUM_FOR_EDICT: Bad pointer" when saving after switching weapons.
- **Cause:** the game data declares the current weapon as if it were a reference to another
  entity; when saving, the engine tries to convert it and, after a weapon switch, the value is out
  of range.
- **Fix:** on entering each map that field is treated as a number. *(hlalpha.c)*

### Crash with many bullet holes · [14]
- **Symptom:** access violation while drawing (`enginegl.exe+0x3881d`).
- **Cause:** the list of surfaces with decals has 500 fixed entries; the engine wrote the new
  entry **before** checking the limit, trashing the memory behind it.
- **Fix:** if the list is full, the extra surface is not drawn that frame. *(patches.h)*

### Crash drawing bullet holes on large faces · [31]
- **Symptom:** the game closed (access violation at `enginegl.exe+0x38882`).
- **Cause:** to draw a decal, the engine copies the wall polygon into a 20-vertex buffer without
  checking how many it has; on faces with more than 16 vertices (116 in the whole alpha) the
  buffer overflowed into the loop variables.
- **Fix:** no decals are drawn on those faces. *(patches.h)*

### "SZ_GetSpace: overflow" with many enemies at once · [19], [35]
- **Symptom:** shooting several enemies at once (a grenade among 3-4 hounds), first an Engine
  Error and, after the first fix, "Illegible server message" and the game was cut off.
- **Cause:** the server's per-frame message buffers are 1 KB and `hl.dll`'s messages do not check
  for space. The first fix ([19]) flushed the buffer when it filled up, but that happened in the
  middle of a message and the client received a broken message.
- **Fix:** the broadcast buffers grow to 16 KB and 4 KB. If a frame does not fit in the message
  to the client, the engine drops it entirely (that frame's effects are lost, nothing is
  corrupted). *(hlalpha.c)* — **Pending:** confirm by repeating the grenade.

### The chapter 1 exit stopped working · [20]
- **Symptom:** at the end of `c1a1a`, past the double door, the map did not change.
- **Cause:** in `hl.dll` the level-change trigger is disabled as soon as you step on it, before
  requesting the change. If the change failed (back when the exit to `c1a1c` was still ignored),
  the trigger stayed disabled, was saved that way and no longer worked in that game.
- **Fix:** on load, disabled level-change triggers are re-enabled as they are in the freshly
  loaded map. A change that does happen loads another map, so a disabled trigger inside a save is
  always a failed change. *(hlalpha.c)*

### The chapter 1 exit could be crossed crouching · [22]
- **Cause:** the `c1a1a` exit trigger is a narrow strip at the top of the door; crouching you went
  under it.
- **Fix:** on entering the map (or loading) the trigger is stretched to the whole door opening.
  *(hlalpha.c)*

### Hang when going to a map that does not exist · [18]
- **Symptom:** the game stays "thinking" at a transition (`c1a1a` → `c1a1c`).
- **Cause:** the alpha does not include `c1a1c` or `c3a3`, but there are exits to them. The engine
  freezes the image and then cannot find the map (error in a window hidden behind the fullscreen
  display, or a frozen image for 60 s).
- **Fix:** the exit to `c1a1c` leads to the next chapter the alpha does have, `c1a2a`, keeping
  health and weapons. It is done in the function the map's trigger calls, changing only the
  destination map name. There is nothing after `c3a3` (`c3a2a` is the last map): that exit is
  ignored with a console warning and play continues. In both cases the other exits of the map
  keep working. *(hlalpha.c)*

---

## Graphics

### Bullet holes flickered · [32]
- **Symptom:** decals (holes, blood) flickered between black and grey without moving.
- **Cause:** the engine alternates the depth range on alternate frames (`gl_ztrick`) and got the
  decal offset from the wall wrong: on every other frame they ended up behind it.
- **Fix:** the offset is always applied towards the camera. *(patches.h)*

### Weapon and grenade shadows · [37]
- **Symptom:** the first-person weapon's shadow floated beside it, and the grenade dragged a
  shadow in flight.
- **Fix:** those two no longer have a shadow *(hlalpha.c)*. Also, as a preference, shadows are
  off by default (`r_shadows 0`); if they are turned back on, the weapon and the grenade still
  have none.

---

## Aiming

### The crosshair was not at the aim point · [15]
- **Symptom:** shots went above the crosshair.
- **Cause:** the crosshair was drawn with the corner of the `+` character at the centre; when the
  320×200 virtual screen was stretched to the real resolution it ended up 20 px low and 6 px to
  the right.
- **Fix:** crosshair moved to the real centre. *(patches.h)*

### Shots drifted depending on where you looked · [16]
- **Symptom:** looking up the shot went low, looking down it went high, and always a bit to the
  right.
- **Cause:** the client sent its angles to the server in one byte (1.4° steps), truncating.
- **Fix:** movement angles travel in 16 bits (0.002° error), as in the final Half-Life; the rest
  of the angles are rounded instead of truncated. *(patches.h)*

### The bullet did not come from the eye · [16]
- **Cause:** `hl.dll` fired from 10 units in front of and 4 below the eye.
- **Fix:** the bullet comes from the player's eye. *(patches.h, on hl.dll)*

### Monsters with a particle field around them · [23]
- **Symptom:** a headcrab in `c1a2b` (and later its corpse) had a strange effect around it.
- **Cause:** if a monster starts stuck in a wall, `hl.dll` prints a map error to the console and
  gives it a particle field as a debugging mark for the mapper.
- **Fix:** the warning stays but the mark does not; on load it is also removed from saves that
  already had it. *(patches.h, on hl.dll + hlalpha.c)*

---

## Artificial intelligence

### Scientists ran on the spot when following you · [28]
- **Symptom:** after pressing E, the scientist kept playing the run animation without moving,
  until you shot him.
- **Cause:** while a friendly monster sees the player, its AI routine ended before running what it
  was doing (following you). A shot gave it an enemy and it took another path.
- **Fix:** in that case the routine goes on to run its task. *(patches.h, on hl.dll)*

---

## Controls

### Keys 1, 2 and 3 switch weapons directly · [30]
- **Before:** the number only preselected the weapon and you had to confirm by firing.
- **Now:** 1 = crowbar, 2 = pistol, 3 = MP5, instantly; the same with next/previous weapon.
  *(patches.h, on hl.dll)*

---

## Configuration

### The HUD was not visible · [12]
- Not a bug: the HUD starts off and is turned on with `sizedown` (the `-` key). `winmm.dll` sends
  it at startup (`+sizedown`). *(hlalpha.c)*

### The mouse did not aim and the keys were the test ones
- **Symptom:** moving the mouse up and down walked instead of looking; the F keys changed the CD
  track and several letters ran test commands (`r_fullbright`, `host_speeds`...).
- **Cause:** the CD's `autoexec.cfg` and `config.cfg` are Valve's development ones.
- **Fix:** our own `autoexec.cfg` with WASD, mouselook (`+mlook`), quicksave on F6/F7, the
  menus on F2–F4 and screenshots on F5. *(valve/autoexec.cfg)*

### Walls not drawn until you got close · [24]
- **Symptom:** in some corridors (e.g. `c1a2b`) a distant wall was not drawn and you could see
  through it; it appeared as you got closer.
- **Cause:** the map's own precomputed visibility (1997) marks that area as not visible from where
  you are. Checked by reading the `.bsp`: the engine does what the data says.
- **Fix:** `r_novis 1`: the world is drawn without using that table. *(hlalpha.c)*

### Doors and other entities invisible until you got close · [38]
- **Symptom:** a door in front of you did not appear until you got closer.
- **Cause:** the same faulty map visibility: the server only sends the entities that table marks
  as visible.
- **Fix:** in single player, the server sends every entity (and the per-frame message grows from
  1 to 4 KB so they fit). Multiplayer is left as it was. *(hlalpha.c)*

### The game stuttered when looking towards some areas · [25]
- **Symptom:** on a modern PC, big fps drops depending on where you look.
- **Cause:** the game used the 3Dfx OpenGL in its folder (MiniGL) on top of nGlide, which emulates
  a 1997 Voodoo on Direct3D 9: about 1,600 polygons cost 30–39 ms per frame.
- **Fix:** the installation moves the `Opengl32.dll` out of the folder, so the graphics card's
  native OpenGL is used (1–2 ms in the same view). *(README, installation)*

### A single executable and "New Game" from the menu · [33]
- **Before:** the game had to be started from a `.bat`, and the menu's "New Game" did not work (it
  asked for a `start` map the alpha does not have).
- **Now:** double-clicking `enginegl.exe` opens the menu, and "New Game" starts at `c1a1`.
  *(hlalpha.c)*

### Desktop resolution · [36]
- **Before:** fixed 800×600.
- **Now:** the game starts at your screen's resolution (for example 1920×1080). The HUD and menus
  are still 320×200 stretched. *(hlalpha.c)*

---

## Known unfixed bugs

- **`changelevel <map>` typed by hand in the console** crashes the game (`hl.dll+0xb92d`), also
  with the original engine and DLL. It does not happen while playing. To change map:
  `map <map>`. [18]
- **The `c3a3` map is missing**: that exit from `c3a2` leads nowhere, because the alpha has
  nothing after it. [18]
