# pr-mediad

> Project-R media detection daemon — identifies physical media and ROMs via udev/libblkid.

**Language:** C · **Status:** Pre-alpha · [Implementation Plan](https://github.com/willzmkv/project-r-os/blob/main/ROADMAP.md#pr-mediad--media-detection-daemon)

## What it does

Monitors udev for disc/USB insertion events. Reads magic bytes and filesystem signatures to identify the game platform (PS1–PS4, Xbox, N64, SNES, etc.). Sends a JSON event over Unix socket to pr-builder.

```
udev event → magic bytes → platform detected → JSON → /run/project-r/pr-builder.sock
```

## Architecture

- `platform_db.h/c` — 23-platform signature database (magic bytes + filesystem + ROM extension)
- `media_detect.h/c` — detection engine (libblkid + direct file reads)
- `ipc.h/c` — Unix socket client (newline-delimited JSON, connects to pr-builder)
- `notify.h/c` — libnotify wrapper for desktop notifications
- `main.c` — udev monitor loop

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build -V
```

## Dependencies

- libudev
- libblkid
- libnotify
- cJSON (vendored in `vendor/`)

## IPC Protocol

pr-mediad is the **client**. pr-builder runs the Unix socket server at `/run/project-r/pr-builder.sock`.

```json
{"platform": "PS1", "device": "/dev/sr0", "mount_point": "/mnt/disc", "rom_hash": null}
```

## License

GPL-3.0
