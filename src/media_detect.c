/* src/media_detect.c */
#include "media_detect.h"
#include "platform_db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>

/* Procura `name` em `dir` sem diferenciar maiúsculas/minúsculas.
 * Necessário porque o isofs do Linux (map=normal, check=strict) expõe nomes
 * ISO9660 sem Rock Ridge/Joliet em minúsculas (SYSTEM.CNF -> system.cnf).
 * Retorna caminho completo alocado (caller faz free) ou NULL. */
static char *find_entry_nocase(const char *dir, const char *name) {
    DIR *d = opendir(dir);
    if (!d) return NULL;

    char *found = NULL;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (strcasecmp(de->d_name, name) != 0) continue;
        size_t len = strlen(dir) + 1 + strlen(de->d_name) + 1;
        found = malloc(len);
        if (found) snprintf(found, len, "%s/%s", dir, de->d_name);
        break;
    }
    closedir(d);
    return found;
}

/* Compara prefixo ignorando espaços/tabs, para que "BOOT2 = cdrom0:..."
 * (formato dos discos PS2 reais) case com o prefixo "BOOT2=". */
static int prefix_matches_ignoring_blanks(const char *text, const char *prefix) {
    const char *t = text, *p = prefix;
    for (;;) {
        while (*t == ' ' || *t == '\t') t++;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') return 1;
        if (*t != *p) return 0;
        t++;
        p++;
    }
}

PlatformID detect_magic_bytes(const char *device_path) {
    if (!device_path) return PLATFORM_UNKNOWN;

    size_t sig_count;
    const DiscSignature *sigs = disc_signatures(&sig_count);

    FILE *f = fopen(device_path, "rb");
    if (!f) return PLATFORM_UNKNOWN;

    PlatformID result = PLATFORM_UNKNOWN;

    for (size_t i = 0; i < sig_count; i++) {
        if (sigs[i].type != SIG_MAGIC_BYTES) continue;

        if (fseek(f, sigs[i].offset, SEEK_SET) != 0) continue;

        uint8_t *buf = malloc(sigs[i].magic_len);
        if (!buf) continue;

        size_t nread = fread(buf, 1, sigs[i].magic_len, f);
        if (nread == sigs[i].magic_len &&
            memcmp(buf, sigs[i].magic, sigs[i].magic_len) == 0) {
            result = sigs[i].platform;
            free(buf);
            break;
        }
        free(buf);
    }

    fclose(f);
    return result;
}

PlatformID detect_filesystem(const char *mount_point) {
    if (!mount_point) return PLATFORM_UNKNOWN;

    size_t sig_count;
    const DiscSignature *sigs = disc_signatures(&sig_count);

    for (size_t i = 0; i < sig_count; i++) {
        if (sigs[i].type == SIG_MAGIC_BYTES) continue;
        if (!sigs[i].filename) continue;

        /* Caminho completo: mount_point + "/" + filename (case-insensitive) */
        char *path = find_entry_nocase(mount_point, sigs[i].filename);
        if (!path) continue;

        struct stat st;
        if (stat(path, &st) != 0) { free(path); continue; }

        if (sigs[i].type == SIG_FILE_EXISTS) {
            free(path);
            return sigs[i].platform;
        }

        if (sigs[i].type == SIG_FILE_PREFIX && sigs[i].prefix) {
            FILE *f = fopen(path, "r");
            free(path);
            if (!f) continue;
            /* lê o começo do arquivo; folga para espaços entre tokens */
            char buf[256];
            size_t r = fread(buf, 1, sizeof(buf) - 1, f);
            fclose(f);
            buf[r] = '\0';
            if (prefix_matches_ignoring_blanks(buf, sigs[i].prefix))
                return sigs[i].platform;
            continue;
        }
        free(path);
    }
    return PLATFORM_UNKNOWN;
}

PlatformID detect_rom_file(const char *filepath) {
    return platform_from_extension(filepath);
}

PlatformID detect_platform(const char *device_path, const char *mount_point) {
    PlatformID id = detect_magic_bytes(device_path);
    if (id != PLATFORM_UNKNOWN) return id;
    id = detect_filesystem(mount_point);
    if (id != PLATFORM_UNKNOWN) return id;
    return PLATFORM_UNKNOWN;
}
