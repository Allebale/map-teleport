# Map Teleport

A mod for the Super Mario Sunshine PC port (and Super Mario Eclipse builds): open a map of Isle Delfino
in a stage and teleport to any level your save file has reached.

- **N** (or D-pad up on a controller; Z and D-pad up, or none, in the Mods menu) opens the map.
- Move between levels with the stick, the D-pad or the arrow keys. **A** (a left click) chooses, **B** (a right click) or N goes back.
- A level with several episodes opens a list of them: the ones its episode select would offer.
- Grey places are not reached yet.

Which levels: the ones this save file has been to since the mod was on, and in Super Mario Sunshine every world
the file has a Shine of. They are kept for each save file of each game, at once. A setting opens every level.

Shadow Mario: in Super Mario Sunshine, while he waits in Delfino Plaza for a chase (Yoshi's Egg after Pinna
Park's episode 4, the Turbo Nozzle at 25 Shines and the Rocket Nozzle at 30), teleporting takes you to the plaza instead, so
the chase isn't missed. In Super Mario Eclipse the same goes for the nozzle chases at 25 and 30 Shines. The map
says so. A setting turns it off.

Going back: B in the episode select a teleport opened takes you back to where you were. The game has no way
back from it, so the mod closes it and loads the level you came from in place of the episode chosen (its
opening skipped; a game without SMS_EV_STAGE_LOADING loads the episode and teleports back from it). A setting
turns it off.

Build: `cmake -S . -B build -DSMS_MODS_OUT=<the game's mods folder> && cmake --build build`.

## Credits

- **Super Mario Sunshine** and its levels, characters and pictures belong to **Nintendo**.
- **Super Mario Eclipse**, its levels, its logo and the pictures of its levels belong to the **Eclipse Team**.
  The descriptions of Eclipse's levels are based on their guide.
- The descriptions of Super Mario Sunshine's levels are based on the **Super Mario Wiki** (mariowiki.com).
- The map and the mod are by Allebale. This is a fan-made mod, not made or endorsed by Nintendo or the Eclipse Team.
