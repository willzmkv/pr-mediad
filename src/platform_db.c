/* src/platform_db.c */
#include "platform_db.h"
#include <string.h>
#include <strings.h>  /* strcasecmp (POSIX) */

/* Tabela de informações por plataforma */
static const PlatformInfo PLATFORMS[PLATFORM_COUNT] = {
    [PLATFORM_UNKNOWN]   = {PLATFORM_UNKNOWN,  "Unknown",        "unknown",   NULL,                  NULL},
    [PLATFORM_PS1]       = {PLATFORM_PS1,      "PlayStation 1",  "ps1",       "duckstation",         NULL},
    [PLATFORM_PS2]       = {PLATFORM_PS2,      "PlayStation 2",  "ps2",       NULL,                  "pcsx2"},
    [PLATFORM_PS3]       = {PLATFORM_PS3,      "PlayStation 3",  "ps3",       NULL,                  "rpcs3"},
    [PLATFORM_PS4]       = {PLATFORM_PS4,      "PlayStation 4",  "ps4",       NULL,                  "shadps4"},
    [PLATFORM_PS5]       = {PLATFORM_PS5,      "PlayStation 5",  "ps5",       NULL,                  "kytyps5"},
    [PLATFORM_N64]       = {PLATFORM_N64,      "Nintendo 64",    "n64",       "mupen64plus_next",    NULL},
    [PLATFORM_GAMECUBE]  = {PLATFORM_GAMECUBE, "GameCube",       "gc",        NULL,                  "dolphin"},
    [PLATFORM_WII]       = {PLATFORM_WII,      "Wii",            "wii",       NULL,                  "dolphin"},
    [PLATFORM_XBOX]      = {PLATFORM_XBOX,     "Xbox",           "xbox",      NULL,                  "xemu"},
    [PLATFORM_XBOX360]   = {PLATFORM_XBOX360,  "Xbox 360",       "xbox360",   NULL,                  "xenia"},
    [PLATFORM_SATURN]    = {PLATFORM_SATURN,   "Sega Saturn",    "saturn",    "mednafen_saturn",     NULL},
    [PLATFORM_DREAMCAST] = {PLATFORM_DREAMCAST,"Dreamcast",      "dreamcast", "flycast",             NULL},
    [PLATFORM_NES]       = {PLATFORM_NES,      "NES",            "nes",       "mesen",               NULL},
    [PLATFORM_SNES]      = {PLATFORM_SNES,     "SNES",           "snes",      "bsnes",               NULL},
    [PLATFORM_GAMEBOY]   = {PLATFORM_GAMEBOY,  "Game Boy",       "gb",        "mgba",                NULL},
    [PLATFORM_GBC]       = {PLATFORM_GBC,      "Game Boy Color", "gbc",       "mgba",                NULL},
    [PLATFORM_GBA]       = {PLATFORM_GBA,      "Game Boy Advance","gba",      "mgba",                NULL},
    [PLATFORM_DS]        = {PLATFORM_DS,       "Nintendo DS",    "nds",       "melonds",             NULL},
    [PLATFORM_3DS]       = {PLATFORM_3DS,      "Nintendo 3DS",   "3ds",       NULL,                  "citra"},
    [PLATFORM_SWITCH]    = {PLATFORM_SWITCH,   "Nintendo Switch","switch",    NULL,                  "eden"},
    [PLATFORM_MEGADRIVE] = {PLATFORM_MEGADRIVE,"Mega Drive",     "megadrive", "genesis_plus_gx",     NULL},
    [PLATFORM_ATARI2600] = {PLATFORM_ATARI2600,"Atari 2600",     "atari2600", "stella2014",          NULL},
};

/* Assinaturas de disco — magic bytes */
static const uint8_t MAGIC_GC[]       = {0xC2, 0x33, 0x9F, 0x3D};
static const uint8_t MAGIC_WII[]      = {0x5D, 0x1C, 0x9E, 0xA3};
static const uint8_t MAGIC_SATURN[]   = {0x53, 0x45, 0x47, 0x41, 0x20,
                                          0x53, 0x45, 0x47, 0x41, 0x53,
                                          0x41, 0x54, 0x55, 0x52, 0x4E}; /* "SEGA SEGASATURN" */
static const uint8_t MAGIC_DC[]       = {0x53, 0x45, 0x47, 0x41, 0x20,
                                          0x53, 0x45, 0x47, 0x41, 0x4B,
                                          0x41, 0x54, 0x41, 0x4E, 0x41}; /* "SEGA SEGAKATANA" */
static const uint8_t MAGIC_XBOX[]     = {0x4D, 0x49, 0x43, 0x52, 0x4F,
                                          0x53, 0x4F, 0x46, 0x54, 0x2A,
                                          0x58, 0x42, 0x4F, 0x58}; /* "MICROSOFT*XBOX" */

static const DiscSignature DISC_SIGS[] = {
    {PLATFORM_GAMECUBE,  SIG_MAGIC_BYTES, 0x1C, MAGIC_GC,     4,    NULL, NULL},
    {PLATFORM_WII,       SIG_MAGIC_BYTES, 0x1C, MAGIC_WII,    4,    NULL, NULL},
    {PLATFORM_SATURN,    SIG_MAGIC_BYTES, 0x00, MAGIC_SATURN, 15,   NULL, NULL},
    {PLATFORM_DREAMCAST, SIG_MAGIC_BYTES, 0x00, MAGIC_DC,     15,   NULL, NULL},
    {PLATFORM_XBOX,      SIG_MAGIC_BYTES, 0x10000, MAGIC_XBOX,14,   NULL, NULL},
    /* File-based signatures */
    {PLATFORM_PS2,       SIG_FILE_PREFIX, 0, NULL, 0, "SYSTEM.CNF", "BOOT2="},
    {PLATFORM_PS1,       SIG_FILE_EXISTS, 0, NULL, 0, "SYSTEM.CNF", NULL},
    {PLATFORM_PS3,       SIG_FILE_EXISTS, 0, NULL, 0, "PS3_DISC.SFB", NULL},
    {PLATFORM_PS4,       SIG_FILE_EXISTS, 0, NULL, 0, "PS4_DISC.sfb", NULL},
    {PLATFORM_PS5,       SIG_FILE_EXISTS, 0, NULL, 0, "PS5_DISC.sfb", NULL},
};

/* Mapeamento de extensões */
static const ExtSignature EXT_SIGS[] = {
    {PLATFORM_N64,       ".z64"}, {PLATFORM_N64,       ".n64"},
    {PLATFORM_N64,       ".v64"},
    {PLATFORM_NES,       ".nes"},
    {PLATFORM_SNES,      ".sfc"}, {PLATFORM_SNES,      ".smc"},
    {PLATFORM_GAMEBOY,   ".gb"},
    {PLATFORM_GBC,       ".gbc"},
    {PLATFORM_GBA,       ".gba"},
    {PLATFORM_MEGADRIVE, ".md"},  {PLATFORM_MEGADRIVE, ".gen"},
    {PLATFORM_ATARI2600, ".a26"},
    {PLATFORM_PS1,       ".cue"},
    {PLATFORM_DS,        ".nds"},
    {PLATFORM_3DS,       ".3ds"},
    {PLATFORM_SWITCH,    ".xci"}, {PLATFORM_SWITCH,    ".nsp"},
};

const PlatformInfo *platform_info(PlatformID id) {
    if (id <= PLATFORM_UNKNOWN || id >= PLATFORM_COUNT) return &PLATFORMS[0];
    return &PLATFORMS[id];
}

const char *platform_name(PlatformID id) {
    return platform_info(id)->name;
}

const DiscSignature *disc_signatures(size_t *count) {
    *count = sizeof(DISC_SIGS) / sizeof(DISC_SIGS[0]);
    return DISC_SIGS;
}

PlatformID platform_from_extension(const char *filepath) {
    if (!filepath) return PLATFORM_UNKNOWN;
    /* encontra a última ocorrência de '.' no caminho */
    const char *ext = strrchr(filepath, '.');
    if (!ext) return PLATFORM_UNKNOWN;
    size_t n = sizeof(EXT_SIGS) / sizeof(EXT_SIGS[0]);
    for (size_t i = 0; i < n; i++) {
        if (strcasecmp(ext, EXT_SIGS[i].ext) == 0)
            return EXT_SIGS[i].platform;
    }
    return PLATFORM_UNKNOWN;
}
