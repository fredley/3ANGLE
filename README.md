# 3Angle

A Pebble watchface. Three markers — hour, minute and second — move around the 12
positions of the clock and connect to form a triangle. The hour, minute and
second hands snap to the nearest 5-minute / 5-second position.

The face is resolution-independent: the ring, markers and triangle are computed
from the screen size, so it renders centred on every Pebble display.

## Supported watches

Built for all current Pebble platforms:

| Platform  | Watch                       | Display          |
|-----------|-----------------------------|------------------|
| `aplite`  | Pebble / Pebble Steel       | 144×168 B&W      |
| `basalt`  | Pebble Time / Time Steel    | 144×168 colour   |
| `chalk`   | Pebble Time Round           | 180×180 colour   |
| `diorite` | Pebble 2 / Pebble 2 HR      | 144×168 B&W      |
| `emery`   | Pebble Time 2               | 200×228 colour   |
| `flint`   | Pebble 2 Duo                | 144×168 B&W      |
| `gabbro`  | Pebble Time 2 (round)       | round colour     |

## Configuration

On colour watches the triangle and background colours are configurable via the
Pebble app (the `configurable` capability + `src/pkjs/index.js` config page).

## Building

The project lives in `3ANGLE-44/` and uses Pebble SDK 4.9+ (needed for the
`emery`, `flint` and `gabbro` platforms).

```sh
cd 3ANGLE-44
source .env/bin/activate
pebble build
pebble install --emulator emery   # or basalt, gabbro, ...
```
