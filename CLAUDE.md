# Notes for Claude working on Map Teleport

- Allebale's mod (see the fork's CLAUDE.md in Allebale/sms-pc-port-remastered for the house rules: commit author,
  settings text, no variables named far or near, ask before releases).
- A release is made by the workflow when `mod/mod.ini`'s version has none yet: raise the version and update
  `RELEASE_NOTES.md`.
- Pictures: `mod/pictures/<name>.png`, 480x270, named after `game->area_name` (lower case, spaces as dashes).
  Super Mario Eclipse levels with none of their own show `eclipse.png`.

## Later (Allebale: "maybe later, not now")

- **Keep achievements on.** `game->warp` turns them off (`port_play_unchecked`, until the next file select). A
  mod changing a normal exit's destination (`SMS_EV_STAGE_LEAVING`) does not: the map could set the destination and
  the player then uses the pause menu's Exit Area. The other way (a game change in the fork so mod warps don't
  count) goes against Chase's rule and is mod-specific code in the game: warn first.
