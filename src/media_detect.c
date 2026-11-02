/* src/media_detect.c */
#include "media_detect.h"
#include "platform_db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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

        /* Monta caminho completo: mount_point + "/" + filename */
        size_t len = strlen(mount_point) + 1 + strlen(sigs[i].filename) + 1;
        char *path = malloc(len);
        if (!path) continue;
        snprintf(path, len, "%s/%s", mount_point, sigs[i].filename);

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
            size_t plen = strlen(sigs[i].prefix);
            char *buf = malloc(plen + 1);
            if (!buf) { fclose(f); continue; }
            size_t r = fread(buf, 1, plen, f);
            fclose(f);
            buf[r] = '\0';
            if (strncmp(buf, sigs[i].prefix, plen) == 0) {
                free(buf);
                return sigs[i].platform;
            }
            free(buf);
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
