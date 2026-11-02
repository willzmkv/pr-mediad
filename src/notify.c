/* src/notify.c */
#include "notify.h"
#include "platform_db.h"
#include <libnotify/notify.h>
#include <stdio.h>

void pr_notify_init(void) {
    if (!notify_init("Project-R")) {
        fprintf(stderr, "pr-mediad: failed to init libnotify\n");
    }
}

void pr_notify(PlatformID platform, const char *body) {
    const char *title = (platform != PLATFORM_UNKNOWN)
        ? platform_name(platform)
        : "Project-R";

    NotifyNotification *n = notify_notification_new(title, body, "media-optical");
    if (!n) return;
    notify_notification_set_timeout(n, 4000); /* 4 segundos */
    GError *err = NULL;
    if (!notify_notification_show(n, &err)) {
        fprintf(stderr, "pr-mediad: notify error: %s\n",
                err ? err->message : "unknown");
        if (err) g_error_free(err);
    }
    g_object_unref(G_OBJECT(n));
}

void pr_notify_cleanup(void) {
    notify_uninit();
}
