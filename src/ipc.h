/* src/ipc.h */
#pragma once
#include "platform_db.h"

#define PR_BUILDER_SOCK "/run/project-r/pr-builder.sock"

typedef struct {
    PlatformID  platform;
    const char *device;       /* "/dev/sr0" */
    const char *mount_point;  /* "/mnt/disc" ou NULL para ROMs */
    const char *rom_hash;     /* MD5 do arquivo ROM, pode ser NULL */
} MediaEvent;

/* Envia evento de mídia para pr-builder via Unix socket.
 * sock_path: caminho do socket (usa PR_BUILDER_SOCK em produção).
 * Retorna 0 em sucesso, -1 em erro. */
int ipc_send_event(const char *sock_path, const MediaEvent *ev);
