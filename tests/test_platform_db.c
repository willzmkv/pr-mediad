/* tests/test_platform_db.c */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "platform_db.h"

static void test_platform_name_unknown(void) {
    assert(strcmp(platform_name(PLATFORM_UNKNOWN), "Unknown") == 0);
    printf("PASS: test_platform_name_unknown\n");
}

static void test_platform_info_n64(void) {
    const PlatformInfo *info = platform_info(PLATFORM_N64);
    assert(info != NULL);
    assert(strcmp(info->esde_name, "n64") == 0);
    assert(strcmp(info->ra_core, "mupen64plus_next") == 0);
    assert(info->standalone == NULL);
    printf("PASS: test_platform_info_n64\n");
}

static void test_platform_info_ps3(void) {
    const PlatformInfo *info = platform_info(PLATFORM_PS3);
    assert(info != NULL);
    assert(strcmp(info->esde_name, "ps3") == 0);
    assert(info->ra_core == NULL);
    assert(strcmp(info->standalone, "rpcs3") == 0);
    printf("PASS: test_platform_info_ps3\n");
}

static void test_platform_from_extension_n64(void) {
    assert(platform_from_extension("/roms/game.z64") == PLATFORM_N64);
    assert(platform_from_extension("/roms/game.n64") == PLATFORM_N64);
    assert(platform_from_extension("/roms/game.v64") == PLATFORM_N64);
    printf("PASS: test_platform_from_extension_n64\n");
}

static void test_platform_from_extension_unknown(void) {
    assert(platform_from_extension("/roms/game.xyz") == PLATFORM_UNKNOWN);
    assert(platform_from_extension("/roms/game") == PLATFORM_UNKNOWN);
    printf("PASS: test_platform_from_extension_unknown\n");
}

int main(void) {
    test_platform_name_unknown();
    test_platform_info_n64();
    test_platform_info_ps3();
    test_platform_from_extension_n64();
    test_platform_from_extension_unknown();
    printf("\nAll platform_db tests passed.\n");
    return 0;
}
