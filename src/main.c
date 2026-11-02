/* src/main.c */
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libudev.h>

#include "platform_db.h"
#include "media_detect.h"
#include "media_event.h"
#include "ipc.h"
#include "notify.h"

static volatile sig_atomic_t running = 1;

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

/* Executa argv[0] (via PATH) sem shell, com stderr descartado.
 * Retorna 0 se o comando terminou com status 0. */
static int run_quiet(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        int devnull = open("/dev/null", O_WRONLY | O_CLOEXEC);
        if (devnull >= 0) dup2(devnull, STDERR_FILENO);
        execvp(argv[0], argv);
        _exit(127);
    }
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) return -1;
    }
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : -1;
}

/* 1 se `path` é ponto de montagem (st_dev difere do diretório pai). */
static int is_mountpoint(const char *path) {
    struct stat st, parent;
    if (stat(path, &st) != 0 || stat(PR_MOUNT_BASE, &parent) != 0) return 0;
    return st.st_dev != parent.st_dev;
}

/* Monta o dispositivo read-only em mount_point. Retorna 0 em sucesso. */
static int mount_device(const char *devnode, const char *mount_point) {
    mkdir(PR_MOUNT_BASE, 0755);
    mkdir(mount_point, 0755);

    char *argv[] = {"mount", "-o", "ro", (char *)devnode, (char *)mount_point, NULL};
    if (run_quiet(argv) != 0) {
        rmdir(mount_point);
        return -1;
    }
    return 0;
}

static void umount_device(const char *mount_point) {
    char *argv[] = {"umount", (char *)mount_point, NULL};
    if (run_quiet(argv) != 0) {
        /* mídia já sumiu (ejeção/remoção física): desmontagem lazy */
        char *lazy[] = {"umount", "-l", (char *)mount_point, NULL};
        if (run_quiet(lazy) != 0)
            fprintf(stderr, "pr-mediad: failed to unmount %s\n", mount_point);
    }
    rmdir(mount_point);
}

static void handle_media_added(const char *devnode) {
    char *mount_point = media_mount_path(PR_MOUNT_BASE, devnode);
    if (!mount_point) return;

    /* eventos "change" repetidos para o mesmo disco: já tratado */
    if (is_mountpoint(mount_point)) {
        free(mount_point);
        return;
    }

    printf("pr-mediad: device added: %s\n", devnode);

    int mounted = (mount_device(devnode, mount_point) == 0);

    PlatformID id = detect_platform(devnode, mounted ? mount_point : NULL);

    if (id == PLATFORM_UNKNOWN) {
        pr_notify(PLATFORM_UNKNOWN,
            "Mídia detectada mas plataforma não identificada.\n"
            "Enviando para análise com Maestro...");
        /* TODO fase 8: enviar para pr-maestro para análise REA */
        if (mounted) umount_device(mount_point);
        free(mount_point);
        return;
    }

    const PlatformInfo *info = platform_info(id);
    printf("pr-mediad: %s identified as %s\n", devnode, info->esde_name);
    char body[128];
    snprintf(body, sizeof(body),
        "Plataforma: %s\nBuscando ferramentas de recompilação...",
        info->name);
    pr_notify(id, body);

    MediaEvent ev = {
        .platform    = id,
        .device      = devnode,
        .mount_point = mounted ? mount_point : "",
        .rom_hash    = NULL,
    };
    if (ipc_send_event(PR_BUILDER_SOCK, &ev) != 0) {
        fprintf(stderr, "pr-mediad: pr-builder unavailable, event dropped\n");
        if (mounted) umount_device(mount_point);
    } else if (mounted) {
        /* pr-builder usa mount_point de forma assíncrona: mantém montado
         * até a mídia ser ejetada/removida (handle_media_removed). */
        printf("pr-mediad: %s stays mounted at %s until removal\n",
               devnode, mount_point);
    }
    free(mount_point);
}

static void handle_media_removed(const char *devnode) {
    char *mount_point = media_mount_path(PR_MOUNT_BASE, devnode);
    if (!mount_point) return;
    if (is_mountpoint(mount_point)) {
        printf("pr-mediad: media removed: %s, unmounting %s\n",
               devnode, mount_point);
        umount_device(mount_point);
    }
    free(mount_point);
}

int main(void) {
    /* journald: uma linha por log, sem esperar o buffer encher */
    setvbuf(stdout, NULL, _IOLBF, 0);

    signal(SIGINT,  handle_signal);
    signal(SIGTERM, handle_signal);

    pr_notify_init();
    printf("pr-mediad: starting\n");

    struct udev *udev = udev_new();
    if (!udev) { fprintf(stderr, "pr-mediad: udev_new failed\n"); return 1; }

    struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!mon) {
        fprintf(stderr, "pr-mediad: udev monitor unavailable\n");
        udev_unref(udev);
        return 1;
    }
    udev_monitor_filter_add_match_subsystem_devtype(mon, "block", "disk");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "block", "partition");
    if (udev_monitor_enable_receiving(mon) < 0) {
        fprintf(stderr, "pr-mediad: cannot receive udev events\n");
        udev_monitor_unref(mon);
        udev_unref(udev);
        return 1;
    }

    int fd = udev_monitor_get_fd(mon);

    while (running) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        struct timeval tv = {.tv_sec = 1, .tv_usec = 0};

        if (select(fd + 1, &fds, NULL, NULL, &tv) <= 0) continue;

        struct udev_device *dev = udev_monitor_receive_device(mon);
        if (!dev) continue;

        const char *devnode = udev_device_get_devnode(dev);
        MediaEventKind kind = classify_udev_event(
            udev_device_get_action(dev),
            udev_device_get_devtype(dev),
            udev_device_get_property_value(dev, "ID_CDROM"),
            udev_device_get_property_value(dev, "ID_CDROM_MEDIA"));

        if (devnode) {
            if (kind == MEDIA_EV_INSERTED)     handle_media_added(devnode);
            else if (kind == MEDIA_EV_REMOVED) handle_media_removed(devnode);
        }

        udev_device_unref(dev);
    }

    udev_monitor_unref(mon);
    udev_unref(udev);
    pr_notify_cleanup();
    printf("pr-mediad: stopped\n");
    return 0;
}
