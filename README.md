# pr-mediad

> Project-R media detection daemon — identifies physical media and ROMs via udev.

**Language:** C17 · **Status:** Pre-alpha (v0.1.0) · [Implementation Plan](https://github.com/willzmkv/project-r-os/blob/main/ROADMAP.md#pr-mediad--media-detection-daemon)

## What it does

Monitors udev for disc/USB insertion events. Reads magic bytes and filesystem signatures to identify the game platform (PS1–PS5, GameCube/Wii, Saturn, Dreamcast, Xbox). Sends a JSON event over a Unix socket to pr-builder and shows a desktop notification.

```
udev event → mount (ro) → magic bytes / marker files → platform → JSON → /run/project-r/pr-builder.sock
```

## Architecture

- `platform_db.h/c` — 22-platform database: ES-DE names, RetroArch core / standalone emulator, disc signatures, ROM extensions
- `media_detect.h/c` — detection engine (raw magic bytes on the device + case-insensitive marker files on the mount)
- `media_event.h/c` — udev event classification (insert / remove / ignore) and mount path naming; no libudev dependency
- `ipc.h/c` — Unix socket client (newline-delimited JSON, connects to pr-builder)
- `notify.h/c` — libnotify wrapper for desktop notifications
- `main.c` — udev monitor loop, mount/unmount handling

### Detection

| Signal | Platforms |
|---|---|
| Magic bytes on the device | GameCube (`0x1C`), Wii (`0x1C`), Saturn / Dreamcast (`0x00`), Xbox (`0x10000`) |
| Marker files on the mounted disc | PS2 (`SYSTEM.CNF` starting with `BOOT2 =`), PS1 (`SYSTEM.CNF` or `PSX.EXE`), PS3 (`PS3_DISC.SFB`), PS4 (`PS4_DISC.SFB`), PS5 (`PS5_DISC.SFB`) |
| ROM file extension (`detect_rom_file`) | N64, NES, SNES, GB/GBC/GBA, DS, 3DS, Switch, Mega Drive, Atari 2600, PS1 (`.cue`) |

Marker files are matched case-insensitively (Linux isofs shows plain ISO9660 names in lower case).

### Media lifecycle

| udev event | Action |
|---|---|
| `add` of a `disk` (USB stick, SD, floppy, optical drive with a disc) | mount read-only at `/mnt/project-r/<dev>`, detect, notify, send event |
| `change` of an optical drive with `ID_CDROM_MEDIA=1` (disc inserted) | same as above |
| `change` of an optical drive without media (eject) / `remove` | unmount `/mnt/project-r/<dev>` |

After a successful send, the media **stays mounted** so pr-builder can read `mount_point`. Unknown media or a failed send unmounts immediately.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build -V
```

Install (Arch/CachyOS): `makepkg -si` with the bundled `PKGBUILD` (source tarball `pr-mediad-<ver>.tar.gz`, e.g. from `git archive --prefix=pr-mediad-0.1.0/`), or `sudo cmake --install build`, then `sudo systemctl enable --now pr-mediad`.

## Dependencies

- libudev (systemd-libs)
- libblkid (util-linux-libs)
- libnotify
- cJSON v1.7.19 (vendored in `vendor/cjson/`, MIT)

## IPC contract

pr-mediad is the **client**. pr-builder is the **server** and owns the socket.

| | |
|---|---|
| Socket | `/run/project-r/pr-builder.sock` (`AF_UNIX`, `SOCK_STREAM`) — `PR_BUILDER_SOCK` in `src/ipc.h` |
| Framing | one connection per event: connect, write **one** compact JSON object followed by `\n`, close. No reply is read. |
| Encoding | UTF-8 JSON |
| Delivery | best effort: if the socket is unavailable the event is logged and dropped (no queue/retry) |
| Directory | `/run/project-r` is created by `pr-mediad-tmpfiles.conf`, owned by `arcade` (pr-builder runs as `arcade`) |

Message (single line on the wire):

```json
{"type":"media_inserted","platform":"n64","device":"/dev/sr0","mount_point":"/mnt/disc","rom_hash":"abc123"}
```

| Field | Type | Value |
|---|---|---|
| `type` | string | always `"media_inserted"` |
| `platform` | string | ES-DE system name: `ps1 ps2 ps3 ps4 ps5 n64 gc wii xbox xbox360 saturn dreamcast nes snes gb gbc gba nds 3ds switch megadrive atari2600` (unidentified media is not sent) |
| `device` | string | block device node, e.g. `"/dev/sr0"` |
| `mount_point` | string | read-only mount of the media (`/mnt/project-r/<dev>` from the daemon), or `""` if it could not be mounted |
| `rom_hash` | string | MD5 hex of the ROM; `""` in v0.1 (not computed yet) |

All five fields are always present and always strings — never JSON `null`. Field order is stable (`type, platform, device, mount_point, rom_hash`) but consumers should not depend on it. The exact bytes are pinned by `tests/test_ipc.c`.

Manual test against a running pr-builder:

```bash
echo '{"type":"media_inserted","platform":"n64","device":"/dev/sr0","mount_point":"/mnt/disc","rom_hash":""}' \
  | socat - UNIX-CONNECT:/run/project-r/pr-builder.sock
```

## License

GPL-3.0
