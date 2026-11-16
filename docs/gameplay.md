# Gameplay (current state)

- **Main menu**: New Game, Load Game, Settings, Quit.
- **Settings**: resolution dropdown and fullscreen checkbox, applied to the window by `Game` and saved to `settings.cfg` in the data directory, restored on the next run.
- **New Game** asks for a save name, writes `saves/<name>.sav` (overwriting a save with the same name) and enters the world scene.
- **Load Game** lists the saves, newest first (the list does not scroll), and enters the world with the chosen one.

## World

- **Pause (Esc)**: shows a menu with the save's name and creation date, a Save button (writes the save again, keeping the previous file as `<name>.sav_old`) and a Main Menu button.
- **Player**: drawn as a cube labelled with its name and state (`Idle` / `Moving`); walks with WASD (diagonals are not faster, stops on release); the camera follows it so it stays at the screen centre. The mouse wheel zooms. A `CharacterStateChanged` event is published when it starts or stops moving.
- **Inventory**: 40 slots of item stacks. A new game's player starts with 200 iron ore. Tab opens the player menu (the world keeps running; Esc, Tab or its X closes it) with tabs Inventory (slot grid), Energy (power produced and consumed, overload warning, the list of generators and of consumers with their kW and whether they run) and Statistics and Logistics (disabled).
- **Build bar (B)**: icon tabs Special / Mines / Factories / Military, each with a drop-up list. Only these can be picked:
  - Headquarters: free, built at once, only one.
  - Mine: 100 iron ore, effort 300, only on an ore tile, only once a Headquarters exists.
- **Workers**: placing the Headquarters brings 100 workers (drawn as cubes, labelled Idle / Working). Idle workers go by themselves to any construction site, walking the precomputed route from the Headquarters, and build it like the player does; a site takes at most effort / 10 workers (30 on a mine). When the site is finished they turn idle and walk back to the Headquarters.
- **Production workers**: a Mine needs 10 workers at it for its full rate (0.5 ore/s); with fewer it produces proportionally slower (5 workers = half), with none it is idle (no workers). Workers are never sent to production by themselves: in the player menu (Tab), the **Workers** tab shows the idle workers and every finished production building with its assigned/needed workers; `+` sends one idle worker there (along the route from the Headquarters), `-` sends one back. A worker doing its job (at its mine or construction site) is inside the building and disappears from the view; it shows up again when it walks away. Workers are saved with their position, state, job and the route they are walking, so a loaded game continues where it was.
  With a selection, a translucent cube previews it under the mouse (green = buildable, red = not); clicking places it; a refused click keeps the selection and shows a stronger red. Buildings are tile-wide cubes: blue when finished, grey with a % while a construction site.
- **Building**: holding the left button over a site within 2 tiles builds it at 10 effort/s (a mine takes 30 s).
- **Mines** produce the ore under them (iron, copper or coal) at 0.5/s into a 10-slot inventory (saved; stops while full). Each finished mine consumes 100 kW.
- **Power**: the Headquarters generates 1000 kW of solar power (no input). When the total demand of finished consumers exceeds the supply, all of them lose power and their buildings go idle (reason: no power) and stop producing until the supply covers the demand again (10 mines run, the 11th idles them all).
- **Building panel**: clicking a finished mine within 2 tiles opens it: the player's inventory above the mine's name, state (with the idle reason), power consumption and inventory; clicking a filled slot moves its stack across. X, Esc or walking away closes it.
- **Terrain**: flat, seed-generated: grass (green), water (blue), iron (grey), copper (orange), coal (black). Three medium ore nodes near the spawn, bigger ones further out. Recreated from the seed on load.
- All icons are the placeholder (fallback) texture. There is no other gameplay yet.

## What a save holds

Name, creation time, world seed and the persistent entities: the Player (position restored; state always saved as `Idle`; inventory) and buildings (type, tile, construction progress, inventory).
