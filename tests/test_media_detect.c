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

/* --- detect_filesystem: simula um disco montado num diretório temporário --- */

static char *make_temp_mount(void) {
    char *dir = strdup("/tmp/test_mnt_XXXXXX");
    assert(mkdtemp(dir) != NULL);
    return dir;
}

static void put_file(const char *dir, const char *name, const char *content) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    FILE *f = fopen(path, "wb");
    assert(f != NULL);
    fputs(content, f);
    fclose(f);
}

static void remove_temp_mount(char *dir, const char *name) {
    if (name) {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", dir, name);
        unlink(path);
    }
    rmdir(dir);
    free(dir);
}

static PlatformID detect_fs_with(const char *name, const char *content) {
    char *dir = make_temp_mount();
    put_file(dir, name, content);
    PlatformID id = detect_filesystem(dir);
    remove_temp_mount(dir, name);
    return id;
}

static void test_detect_fs_ps2_exact_prefix(void) {
    assert(detect_fs_with("SYSTEM.CNF", "BOOT2=cdrom0:\\SLUS_200.62;1\r\n")
           == PLATFORM_PS2);
    printf("PASS: test_detect_fs_ps2_exact_prefix\n");
}

/* Discos PS2 reais usam espaços: "BOOT2 = cdrom0:\SLUS_xxx.xx;1" */
static void test_detect_fs_ps2_spaced_prefix(void) {
    assert(detect_fs_with("SYSTEM.CNF",
               "BOOT2 = cdrom0:\\SLUS_200.62;1\r\nVER = 1.00\r\nVMODE = NTSC\r\n")
           == PLATFORM_PS2);
    printf("PASS: test_detect_fs_ps2_spaced_prefix\n");
}

static void test_detect_fs_ps1_system_cnf(void) {
    assert(detect_fs_with("SYSTEM.CNF", "BOOT = cdrom:\\SCUS_944.55;1\r\n")
           == PLATFORM_PS1);
    printf("PASS: test_detect_fs_ps1_system_cnf\n");
}

/* isofs do Linux (map=normal, check=strict) expõe nomes ISO9660 sem
 * Rock Ridge/Joliet em minúsculas: SYSTEM.CNF aparece como system.cnf */
static void test_detect_fs_lowercase_names(void) {
    assert(detect_fs_with("system.cnf", "BOOT = cdrom:\\SCUS_944.55;1\r\n")
           == PLATFORM_PS1);
    assert(detect_fs_with("system.cnf", "BOOT2 = cdrom0:\\SLUS_200.62;1\r\n")
           == PLATFORM_PS2);
    printf("PASS: test_detect_fs_lowercase_names\n");
}

/* Discos PS1 antigos não têm SYSTEM.CNF e bootam PSX.EXE direto */
static void test_detect_fs_ps1_psx_exe(void) {
    assert(detect_fs_with("PSX.EXE", "PS-X EXE") == PLATFORM_PS1);
    printf("PASS: test_detect_fs_ps1_psx_exe\n");
}

static void test_detect_fs_ps3(void) {
    assert(detect_fs_with("PS3_DISC.SFB", ".SFB") == PLATFORM_PS3);
    printf("PASS: test_detect_fs_ps3\n");
}

static void test_detect_fs_unknown(void) {
    char *dir = make_temp_mount();
    assert(detect_filesystem(dir) == PLATFORM_UNKNOWN);
    remove_temp_mount(dir, NULL);
    assert(detect_filesystem(NULL) == PLATFORM_UNKNOWN);
    assert(detect_filesystem("/nonexistent/mount") == PLATFORM_UNKNOWN);
    assert(detect_fs_with("README.TXT", "hello") == PLATFORM_UNKNOWN);
    printf("PASS: test_detect_fs_unknown\n");
}

static void test_detect_platform_chain(void) {
    char *dev = make_temp_gc_disc();
    char *dir = make_temp_mount();
    put_file(dir, "PS3_DISC.SFB", ".SFB");
    /* magic bytes têm prioridade sobre arquivos */
    assert(detect_platform(dev, dir) == PLATFORM_GAMECUBE);
    /* sem magic, cai para o filesystem */
    assert(detect_platform("/nonexistent/dev", dir) == PLATFORM_PS3);
    assert(detect_platform(NULL, dir) == PLATFORM_PS3);
    assert(detect_platform("/nonexistent/dev", NULL) == PLATFORM_UNKNOWN);
    remove_temp_mount(dir, "PS3_DISC.SFB");
    unlink(dev);
    free(dev);
    printf("PASS: test_detect_platform_chain\n");
}

int main(void) {
    test_detect_magic_gamecube();
    test_detect_magic_unknown();
    test_detect_rom_extension();
    test_detect_fs_ps2_exact_prefix();
    test_detect_fs_ps2_spaced_prefix();
    test_detect_fs_ps1_system_cnf();
    test_detect_fs_lowercase_names();
    test_detect_fs_ps1_psx_exe();
    test_detect_fs_ps3();
    test_detect_fs_unknown();
    test_detect_platform_chain();
    printf("\nAll media_detect tests passed.\n");
    return 0;
}
