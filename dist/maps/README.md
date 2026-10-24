# Map files (`.map`)

Maps are plain text so anyone on the team can edit them without touching C++.
The game loads `maps/cave.map` at startup. If something is wrong with a map,
the game stops with an error that names the file and line.

## Example

```
; comments start with a semicolon
tileset assets/tileset.png

;    char  index  collision  name
tile ,     2      walk       grass
tile ~     5      solid      water
tile T     6      solid      tree

spawn 2 1

map
TTTTT
T,,,T
T,~,T
TTTTT
```

## Lines

| Line | Meaning |
| --- | --- |
| `tileset <path>` | The tileset image, relative to the `dist/` folder. |
| `tile <char> <index> <walk\|solid> [name]` | Defines what a map character means. `index` is which 16×16 tile in the tileset to draw, counting left-to-right then top-to-bottom from 0. `solid` tiles block the player. The name is just for debugging (it shows in the F3 overlay). |
| `spawn <column> <row>` | Tile the player starts on. Counts from 0 at the top-left. Must be a `walk` tile. |
| `map` | Everything after this line is the map, one character per tile. |

## Rules

- Every map row must be the same width.
- Every character used in the map needs a `tile` line.
- `;` can't be a tile character (it starts a comment), and neither can a space.
- The edge of the map always blocks movement, so you don't need a solid border
  for collision (but it usually looks better to have one).

## Tileset images

- Tiles are 16×16 pixels, packed in a grid with no gaps or padding.
- The image width and height must be multiples of 16.
- Use PNG with transparency.
