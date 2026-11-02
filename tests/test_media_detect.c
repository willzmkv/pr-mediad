/* tests/test_media_detect.c */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "media_detect.h"
#include "platform_db.h"

/* Cria arquivo temporário com magic bytes do GameCube no offset 0x1C */
static char *make_temp_gc_disc(void) {
    char *path = strdup("/tmp/test_gc_XXXXXX");
    int fd = mkstemp(path);
    /* preenche até offset 0x1C com zeros */
    uint8_t buf[0x20];
    memset(buf, 0, sizeof(buf));
    /* magic GameCube: C2 33 9F 3D */
    buf[0x1C] = 0xC2; buf[0x1D] = 0x33;
    buf[0x1E] = 0x9F; buf[0x1F] = 0x3D;
    write(fd, buf, sizeof(buf));
    close(fd);
    return path;
}

static void test_detect_magic_gamecube(void) {
    char *path = make_temp_gc_disc();
    PlatformID id = detect_magic_bytes(path);
    assert(id == PLATFORM_GAMECUBE);
    unlink(path);
    free(path);
    printf("PASS: test_detect_magic_gamecube\n");
}

static void test_detect_magic_unknown(void) {
    char *path = strdup("/tmp/test_unk_XXXXXX");
    int fd = mkstemp(path);
    uint8_t buf[0x20];
    memset(buf, 0xFF, sizeof(buf));
    write(fd, buf, sizeof(buf));
    close(fd);
    PlatformID id = detect_magic_bytes(path);
    assert(id == PLATFORM_UNKNOWN);
    unlink(path);
    free(path);
    printf("PASS: test_detect_magic_unknown\n");
}

static void test_detect_rom_extension(void) {
    assert(detect_rom_file("/roms/mario.z64") == PLATFORM_N64);
    assert(detect_rom_file("/roms/sonic.md")  == PLATFORM_MEGADRIVE);
    assert(detect_rom_file("/roms/foo.xyz")   == PLATFORM_UNKNOWN);
    printf("PASS: test_detect_rom_extension\n");
}

int main(void) {
    test_detect_magic_gamecube();
    test_detect_magic_unknown();
    test_detect_rom_extension();
    printf("\nAll media_detect tests passed.\n");
    return 0;
}
