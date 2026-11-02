/* src/media_event.c */
#include "media_event.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_one(const char *v) {
    return v && strcmp(v, "1") == 0;
}

MediaEventKind classify_udev_event(const char *action, const char *devtype,
                                   const char *id_cdrom,
                                   const char *id_cdrom_media) {
    if (!action || !devtype) return MEDIA_EV_IGNORE;
    if (strcmp(devtype, "disk") != 0) return MEDIA_EV_IGNORE;

    int optical   = is_one(id_cdrom);
    int has_media = is_one(id_cdrom_media);

    if (strcmp(action, "remove") == 0) return MEDIA_EV_REMOVED;

    if (strcmp(action, "add") == 0) {
        /* drive óptico vazio aparecendo não é inserção de mídia */
        if (optical && !has_media) return MEDIA_EV_IGNORE;
        return MEDIA_EV_INSERTED;
    }

    if (strcmp(action, "change") == 0 && optical)
        return has_media ? MEDIA_EV_INSERTED : MEDIA_EV_REMOVED;

    return MEDIA_EV_IGNORE;
}

char *media_mount_path(const char *base, const char *devnode) {
    if (!base || !devnode) return NULL;

    const char *name = strrchr(devnode, '/');
    name = name ? name + 1 : devnode;
    if (name[0] == '\0' || strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return NULL;

    size_t len = strlen(base) + 1 + strlen(name) + 1;
    char *path = malloc(len);
    if (!path) return NULL;
    snprintf(path, len, "%s/%s", base, name);
    return path;
}
