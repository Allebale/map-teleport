# Map Teleport

A mod for the Super Mario Sunshine PC port (and Super Mario Eclipse builds): open a map of Isle Delfino
in a stage and teleport to any level your save file has reached.

- **M** (or D-pad down on a controller; Z and D-pad down, or none, in the Mods menu) opens the map.
- Move between levels with the stick, the D-pad or the arrow keys. **A** or Enter chooses, **B**, Escape or M goes back.
- A level with several episodes opens a list of them: the ones its episode select would offer.
- Grey places are not reached yet.

Which levels: the ones this save file has been to since the mod was on, and in Super Mario Sunshine every world
the file has a Shine of. They are kept with the save file (saved when the game saves).

Build: `cmake -S . -B build -DSMS_MODS_OUT=<the game's mods folder> && cmake --build build`.
