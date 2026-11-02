/* src/media_event.h — classificação de eventos udev (sem dependência de libudev) */
#pragma once

#define PR_MOUNT_BASE "/mnt/project-r"

typedef enum {
    MEDIA_EV_IGNORE = 0,
    MEDIA_EV_INSERTED,   /* mídia disponível: detectar plataforma */
    MEDIA_EV_REMOVED,    /* mídia ejetada / dispositivo removido: desmontar */
} MediaEventKind;

/* Classifica um evento udev de block device.
 * action:         udev ACTION ("add", "change", "remove", ...)
 * devtype:        DEVTYPE ("disk" ou "partition")
 * id_cdrom:       propriedade ID_CDROM ("1" em drives ópticos) ou NULL
 * id_cdrom_media: propriedade ID_CDROM_MEDIA ("1" com disco presente) ou NULL
 *
 * Drives ópticos já presentes geram "change" (não "add") quando um disco é
 * inserido ou ejetado; pendrives/SD/floppy geram "add"/"remove". */
MediaEventKind classify_udev_event(const char *action, const char *devtype,
                                   const char *id_cdrom,
                                   const char *id_cdrom_media);

/* Ponto de montagem do dispositivo: "<base>/<basename(devnode)>".
 * Retorna string alocada (caller faz free) ou NULL se inválido. */
char *media_mount_path(const char *base, const char *devnode);
