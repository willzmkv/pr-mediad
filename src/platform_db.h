/* src/platform_db.h */
#pragma once
#include <stdint.h>
#include <stddef.h>

typedef enum {
    PLATFORM_UNKNOWN = 0,
    PLATFORM_PS1, PLATFORM_PS2, PLATFORM_PS3, PLATFORM_PS4, PLATFORM_PS5,
    PLATFORM_N64, PLATFORM_GAMECUBE, PLATFORM_WII,
    PLATFORM_XBOX, PLATFORM_XBOX360,
    PLATFORM_SATURN, PLATFORM_DREAMCAST,
    PLATFORM_NES, PLATFORM_SNES,
    PLATFORM_GAMEBOY, PLATFORM_GBC, PLATFORM_GBA,
    PLATFORM_DS, PLATFORM_3DS, PLATFORM_SWITCH,
    PLATFORM_MEGADRIVE, PLATFORM_ATARI2600,
    PLATFORM_COUNT
} PlatformID;

typedef struct {
    PlatformID  id;
    const char *name;        /* "Nintendo 64" */
    const char *esde_name;   /* "n64" */
    const char *ra_core;     /* "mupen64plus_next" ou NULL */
    const char *standalone;  /* "rpcs3" ou NULL */
} PlatformInfo;

typedef enum {
    SIG_MAGIC_BYTES,  /* bytes crus em offset fixo no dispositivo */
    SIG_FILE_EXISTS,  /* arquivo existe no ponto de montagem */
    SIG_FILE_PREFIX,  /* conteúdo de arquivo começa com string */
} SigType;

typedef struct {
    PlatformID     platform;
    SigType        type;
    long           offset;       /* SIG_MAGIC_BYTES: offset em bytes */
    const uint8_t *magic;        /* SIG_MAGIC_BYTES: bytes esperados */
    size_t         magic_len;    /* SIG_MAGIC_BYTES: tamanho */
    const char    *filename;     /* SIG_FILE_EXISTS/PREFIX: path relativo */
    const char    *prefix;       /* SIG_FILE_PREFIX: prefixo esperado */
} DiscSignature;

typedef struct {
    PlatformID  platform;
    const char *ext;   /* ".z64", ".nes", etc. */
} ExtSignature;

/* Identifica plataforma de ROM por extensão do arquivo */
PlatformID platform_from_extension(const char *filepath);

/* Retorna struct com info da plataforma */
const PlatformInfo *platform_info(PlatformID id);

/* Nome legível para logs */
const char *platform_name(PlatformID id);

/* Acesso às tabelas de assinaturas (para media_detect.c) */
const DiscSignature *disc_signatures(size_t *count);
