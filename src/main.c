/* src/main.c */
#define _GNU_SOURCE /* asprintf */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <libudev.h>

#include "platform_db.h"
#include "media_detect.h"
#include "ipc.h"
#include "notify.h"

static volatile int running = 1;

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

/* Monta o dispositivo em /mnt/project-r/<devname> e retorna o path.
 * Caller deve free() o resultado. */
static char *mount_device(const char *devnode) {
    const char *base = strrchr(devnode, '/');
    base = base ? base + 1 : devnode;

    char *mnt = NULL;
    if (asprintf(&mnt, "/mnt/project-r/%s", base) < 0) return NULL;

    mkdir("/mnt/project-r", 0755);
    mkdir(mnt, 0755);

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "mount -o ro %s %s 2>/dev/null", devnode, mnt);
    if (system(cmd) != 0) {
        free(mnt);
        return NULL;
    }
    return mnt;
}

static void umount_device(const char *mount_point) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "umount %s 2>/dev/null", mount_point);
    system(cmd);
    rmdir(mount_point);
}

static void handle_media_added(const char *devnode) {
    printf("pr-mediad: device added: %s\n", devnode);

    char *mount_point = mount_device(devnode);

    PlatformID id = detect_platform(devnode, mount_point);

    if (id == PLATFORM_UNKNOWN) {
        pr_notify(PLATFORM_UNKNOWN,
            "Mídia detectada mas plataforma não identificada.\n"
            "Enviando para análise com Maestro...");
        /* TODO fase 8: enviar para pr-maestro para análise REA */
        if (mount_point) { umount_device(mount_point); free(mount_point); }
        return;
    }

    const PlatformInfo *info = platform_info(id);
    char body[128];
    snprintf(body, sizeof(body),
        "Plataforma: %s\nBuscando ferramentas de recompilação...",
        info->name);
    pr_notify(id, body);

    MediaEvent ev = {
        .platform    = id,
        .device      = devnode,
        .mount_point = mount_point ? mount_point : "",
        .rom_hash    = NULL,
    };
    if (ipc_send_event(PR_BUILDER_SOCK, &ev) != 0) {
        fprintf(stderr, "pr-mediad: pr-builder unavailable, event dropped\n");
    }

    if (mount_point) { umount_device(mount_point); free(mount_point); }
}

int main(void) {
    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    pr_notify_init();
    printf("pr-mediad: starting\n");

    struct udev *udev = udev_new();
    if (!udev) { fprintf(stderr, "pr-mediad: udev_new failed\n"); return 1; }

    struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "block", "disk");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "block", "partition");
    udev_monitor_enable_receiving(mon);

    int fd = udev_monitor_get_fd(mon);

    while (running) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        struct timeval tv = {.tv_sec = 1, .tv_usec = 0};

        if (select(fd + 1, &fds, NULL, NULL, &tv) <= 0) continue;

        struct udev_device *dev = udev_monitor_receive_device(mon);
        if (!dev) continue;

        const char *action  = udev_device_get_action(dev);
        const char *devnode = udev_device_get_devnode(dev);
        const char *devtype = udev_device_get_devtype(dev);

        if (action && devnode && devtype &&
            strcmp(action, "add") == 0 &&
            strcmp(devtype, "disk") == 0) {
            handle_media_added(devnode);
        }

        udev_device_unref(dev);
    }

    udev_monitor_unref(mon);
    udev_unref(udev);
    pr_notify_cleanup();
    printf("pr-mediad: stopped\n");
    return 0;
}
