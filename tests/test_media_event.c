/* tests/test_media_event.c */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "media_event.h"

static void test_classify_usb_disk_add(void) {
    /* pendrive / SD / floppy: "add" do disco inteiro */
    assert(classify_udev_event("add", "disk", NULL, NULL) == MEDIA_EV_INSERTED);
    printf("PASS: test_classify_usb_disk_add\n");
}

static void test_classify_optical_change_with_media(void) {
    /* disco inserido num drive óptico já presente: "change" + ID_CDROM_MEDIA=1 */
    assert(classify_udev_event("change", "disk", "1", "1") == MEDIA_EV_INSERTED);
    printf("PASS: test_classify_optical_change_with_media\n");
}

static void test_classify_optical_add(void) {
    /* drive óptico conectado já com disco */
    assert(classify_udev_event("add", "disk", "1", "1") == MEDIA_EV_INSERTED);
    /* drive óptico vazio: não é inserção de mídia */
    assert(classify_udev_event("add", "disk", "1", NULL) == MEDIA_EV_IGNORE);
    printf("PASS: test_classify_optical_add\n");
}

static void test_classify_removal(void) {
    /* ejeção: "change" sem ID_CDROM_MEDIA */
    assert(classify_udev_event("change", "disk", "1", NULL) == MEDIA_EV_REMOVED);
    assert(classify_udev_event("change", "disk", "1", "0") == MEDIA_EV_REMOVED);
    /* dispositivo removido */
    assert(classify_udev_event("remove", "disk", NULL, NULL) == MEDIA_EV_REMOVED);
    assert(classify_udev_event("remove", "disk", "1", NULL) == MEDIA_EV_REMOVED);
    printf("PASS: test_classify_removal\n");
}

static void test_classify_ignored(void) {
    /* "change" de disco não-óptico (ex.: tabela de partições relida) */
    assert(classify_udev_event("change", "disk", NULL, NULL) == MEDIA_EV_IGNORE);
    /* partições continuam ignoradas, como no plano */
    assert(classify_udev_event("add", "partition", NULL, NULL) == MEDIA_EV_IGNORE);
    assert(classify_udev_event("remove", "partition", NULL, NULL) == MEDIA_EV_IGNORE);
    assert(classify_udev_event("bind", "disk", NULL, NULL) == MEDIA_EV_IGNORE);
    assert(classify_udev_event(NULL, "disk", NULL, NULL) == MEDIA_EV_IGNORE);
    assert(classify_udev_event("add", NULL, NULL, NULL) == MEDIA_EV_IGNORE);
    printf("PASS: test_classify_ignored\n");
}

static void test_mount_path(void) {
    char *p = media_mount_path("/mnt/project-r", "/dev/sr0");
    assert(p && strcmp(p, "/mnt/project-r/sr0") == 0);
    free(p);
    p = media_mount_path("/mnt/project-r", "/dev/sdb");
    assert(p && strcmp(p, "/mnt/project-r/sdb") == 0);
    free(p);
    p = media_mount_path("/mnt/project-r", "sr1");
    assert(p && strcmp(p, "/mnt/project-r/sr1") == 0);
    free(p);
    assert(media_mount_path("/mnt/project-r", NULL) == NULL);
    assert(media_mount_path("/mnt/project-r", "") == NULL);
    assert(media_mount_path("/mnt/project-r", "/dev/") == NULL);
    assert(media_mount_path("/mnt/project-r", "/dev/..") == NULL);
    assert(media_mount_path(NULL, "/dev/sr0") == NULL);
    printf("PASS: test_mount_path\n");
}

int main(void) {
    test_classify_usb_disk_add();
    test_classify_optical_change_with_media();
    test_classify_optical_add();
    test_classify_removal();
    test_classify_ignored();
    test_mount_path();
    printf("\nAll media_event tests passed.\n");
    return 0;
}
