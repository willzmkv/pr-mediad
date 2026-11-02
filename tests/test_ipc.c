/* tests/test_ipc.c */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include "ipc.h"
#include "platform_db.h"

#define TEST_SOCK "/tmp/test_pr_builder.sock"

static char received_msg[512];
static int  server_done = 0;

/* sincroniza: o cliente só conecta depois do listen() */
static pthread_mutex_t ready_mtx  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  ready_cond = PTHREAD_COND_INITIALIZER;
static int             server_ready = 0;

static void *mock_server(void *arg) {
    (void)arg;
    int srv = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = {.sun_family = AF_UNIX};
    strncpy(addr.sun_path, TEST_SOCK, sizeof(addr.sun_path)-1);
    unlink(TEST_SOCK);
    bind(srv, (struct sockaddr*)&addr, sizeof(addr));
    listen(srv, 1);

    pthread_mutex_lock(&ready_mtx);
    server_ready = 1;
    pthread_cond_signal(&ready_cond);
    pthread_mutex_unlock(&ready_mtx);

    int cli = accept(srv, NULL, NULL);
    /* lê até o cliente fechar a conexão (uma mensagem por conexão) */
    size_t total = 0;
    ssize_t n;
    while (total < sizeof(received_msg) - 1 &&
           (n = read(cli, received_msg + total,
                     sizeof(received_msg) - 1 - total)) > 0) {
        total += (size_t)n;
    }
    received_msg[total] = '\0';
    close(cli);
    close(srv);
    unlink(TEST_SOCK);
    server_done = 1;
    return NULL;
}

static void start_server(pthread_t *tid) {
    memset(received_msg, 0, sizeof(received_msg));
    server_done  = 0;
    server_ready = 0;
    pthread_create(tid, NULL, mock_server, NULL);
    pthread_mutex_lock(&ready_mtx);
    while (!server_ready) pthread_cond_wait(&ready_cond, &ready_mtx);
    pthread_mutex_unlock(&ready_mtx);
    usleep(50000); /* folga extra, como no plano */
}

static void test_ipc_send_event(void) {
    pthread_t tid;
    start_server(&tid);

    MediaEvent ev = {
        .platform    = PLATFORM_N64,
        .device      = "/dev/sr0",
        .mount_point = "/mnt/disc",
        .rom_hash    = "abc123",
    };

    int ret = ipc_send_event(TEST_SOCK, &ev);
    assert(ret == 0);
    pthread_join(tid, NULL);
    assert(server_done == 1);
    /* verifica JSON recebido contém campos esperados */
    assert(strstr(received_msg, "\"platform\":\"n64\"") != NULL);
    assert(strstr(received_msg, "\"device\":\"/dev/sr0\"") != NULL);
    assert(strstr(received_msg, "media_inserted") != NULL);
    printf("PASS: test_ipc_send_event\n");
}

/* Contrato com pr-builder: uma linha JSON compacta terminada em '\n',
 * campos e ordem exatamente como no plano. */
static void test_ipc_wire_format_exact(void) {
    pthread_t tid;
    start_server(&tid);

    MediaEvent ev = {
        .platform    = PLATFORM_N64,
        .device      = "/dev/sr0",
        .mount_point = "/mnt/disc",
        .rom_hash    = "abc123",
    };
    assert(ipc_send_event(TEST_SOCK, &ev) == 0);
    pthread_join(tid, NULL);
    assert(strcmp(received_msg,
        "{\"type\":\"media_inserted\",\"platform\":\"n64\","
        "\"device\":\"/dev/sr0\",\"mount_point\":\"/mnt/disc\","
        "\"rom_hash\":\"abc123\"}\n") == 0);
    printf("PASS: test_ipc_wire_format_exact\n");
}

/* Campos NULL viram string vazia (nunca JSON null) */
static void test_ipc_null_fields_are_empty_strings(void) {
    pthread_t tid;
    start_server(&tid);

    MediaEvent ev = {
        .platform    = PLATFORM_PS2,
        .device      = "/dev/sr0",
        .mount_point = NULL,
        .rom_hash    = NULL,
    };
    assert(ipc_send_event(TEST_SOCK, &ev) == 0);
    pthread_join(tid, NULL);
    assert(strcmp(received_msg,
        "{\"type\":\"media_inserted\",\"platform\":\"ps2\","
        "\"device\":\"/dev/sr0\",\"mount_point\":\"\","
        "\"rom_hash\":\"\"}\n") == 0);
    printf("PASS: test_ipc_null_fields_are_empty_strings\n");
}

static void test_ipc_no_server(void) {
    unlink(TEST_SOCK);
    MediaEvent ev = {.platform = PLATFORM_N64, .device = "/dev/sr0"};
    assert(ipc_send_event(TEST_SOCK, &ev) == -1);
    assert(ipc_send_event(TEST_SOCK, NULL) == -1);
    assert(ipc_send_event(NULL, &ev) == -1);
    printf("PASS: test_ipc_no_server\n");
}

int main(void) {
    test_ipc_send_event();
    test_ipc_wire_format_exact();
    test_ipc_null_fields_are_empty_strings();
    test_ipc_no_server();
    printf("\nAll ipc tests passed.\n");
    return 0;
}
