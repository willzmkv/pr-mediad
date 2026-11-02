/* src/notify.h */
#pragma once
#include "platform_db.h"

/* Inicializa libnotify. Chamar uma vez no início do daemon. */
void pr_notify_init(void);

/* Exibe notificação ao usuário.
 * platform: plataforma detectada (PLATFORM_UNKNOWN = genérico)
 * body: texto da mensagem */
void pr_notify(PlatformID platform, const char *body);

/* Finaliza libnotify. Chamar ao encerrar o daemon. */
void pr_notify_cleanup(void);
