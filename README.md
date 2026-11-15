# Map Teleport

A mod for the Super Mario Sunshine PC port (and Super Mario Eclipse builds): open a map of Isle Delfino
in a stage and teleport to any level your save file has reached.

- **N** (or D-pad up on a controller; Z and D-pad up, or none, in the Mods menu) opens the map.
- Move between levels with the stick, the D-pad or the arrow keys. **A** (a left click) chooses, **B** (a right click) or N goes back.
- A level with several episodes opens a list of them: the ones its episode select would offer.
- Grey places are not reached yet.

Which levels: the ones this save file has been to since the mod was on, and in Super Mario Sunshine every world
the file has a Shine of. They are kept for each save file of each game, at once. A setting opens every level.

Build: `cmake -S . -B build -DSMS_MODS_OUT=<the game's mods folder> && cmake --build build`.
