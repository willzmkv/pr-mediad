/* src/ipc.c */
#include "ipc.h"
#include "platform_db.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

/* Escreve o buffer inteiro; MSG_NOSIGNAL evita SIGPIPE (que mataria o
 * daemon) se o pr-builder fechar a conexão antes da hora. */
static int send_all(int fd, const char *buf, size_t len) {
    while (len > 0) {
        ssize_t n = send(fd, buf, len, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        buf += n;
        len -= (size_t)n;
    }
    return 0;
}

int ipc_send_event(const char *sock_path, const MediaEvent *ev) {
    if (!sock_path || !ev) return -1;

    /* Monta JSON */
    cJSON *root = cJSON_CreateObject();
    if (!root) return -1;
    cJSON_AddStringToObject(root, "type",        "media_inserted");
    cJSON_AddStringToObject(root, "platform",    platform_info(ev->platform)->esde_name);
    cJSON_AddStringToObject(root, "device",      ev->device      ? ev->device      : "");
    cJSON_AddStringToObject(root, "mount_point", ev->mount_point ? ev->mount_point : "");
    cJSON_AddStringToObject(root, "rom_hash",    ev->rom_hash    ? ev->rom_hash    : "");

    char *msg = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!msg) return -1;

    /* JSON + newline (protocolo newline-delimited) */
    size_t len = strlen(msg);
    char *line = realloc(msg, len + 2);
    if (!line) { free(msg); return -1; }
    msg = line;
    msg[len]   = '\n';
    msg[len+1] = '\0';

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    if (strlen(sock_path) >= sizeof(addr.sun_path)) {
        fprintf(stderr, "pr-mediad: socket path too long: %s\n", sock_path);
        free(msg);
        return -1;
    }
    strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

    /* Conecta ao socket */
    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) { free(msg); return -1; }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "pr-mediad: pr-builder socket not available: %s\n",
                strerror(errno));
        close(fd);
        free(msg);
        return -1;
    }

    int ret = send_all(fd, msg, len + 1);
    if (ret != 0)
        fprintf(stderr, "pr-mediad: failed to send event: %s\n", strerror(errno));
    close(fd);
    free(msg);
    return ret;
}
