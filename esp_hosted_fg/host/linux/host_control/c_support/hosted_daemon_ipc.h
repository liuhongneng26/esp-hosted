/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Espressif Systems Wireless LAN device driver
 *
 * Copyright (C) 2015-2024 Espressif Systems (Shanghai) PTE LTD
 *
 * IPC server header for hosted_daemon - provides IPC interface for hosted_cli
 */

#ifndef __HOSTED_DAEMON_IPC_H__
#define __HOSTED_DAEMON_IPC_H__

#include <stdint.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>

#define IPC_SOCKET_PATH "/var/run/hosted.sock"
#define MAX_IPC_PAYLOAD_SIZE 4096
#define MAX_CLI_CONNECTIONS 10
#define MAX_EVENT_SUBSCRIPTIONS 32

/* IPC Command IDs */
#define IPC_CMD_EXECUTE       0x01
#define IPC_CMD_RESPONSE      0x02
#define IPC_CMD_EVENT         0x03
#define IPC_CMD_ERROR         0x04
#define IPC_CMD_SUBSCRIBE     0x05
#define IPC_CMD_UNSUBSCRIBE   0x06
#define IPC_CMD_PING          0x07

/* IPC Message Header */
typedef struct {
    uint32_t cmd_id;
    uint32_t length;
    uint8_t payload[MAX_IPC_PAYLOAD_SIZE];
} ipc_msg_t;

/* Forward declaration */
typedef struct daemon_ipc_ctx daemon_ipc_ctx_t;

/* CLI Client Context */
typedef struct cli_client {
    int fd;
    int subscribed_events[MAX_EVENT_SUBSCRIPTIONS];
    pthread_t reader_thread;
    int active;
    daemon_ipc_ctx_t *ctx;
} cli_client_t;

/* Daemon IPC Context */
struct daemon_ipc_ctx {
    int server_fd;
    cli_client_t clients[MAX_CLI_CONNECTIONS];
    pthread_mutex_t clients_lock;
    pthread_t accept_thread;
    int active;
};

/* IPC Server API */
int ipc_server_init(daemon_ipc_ctx_t *ctx);
int ipc_server_start(daemon_ipc_ctx_t *ctx);
void ipc_server_stop(daemon_ipc_ctx_t *ctx);
int ipc_send_to_client(cli_client_t *client, uint32_t cmd_id, const char *payload, uint32_t len);
int ipc_broadcast_event(daemon_ipc_ctx_t *ctx, int event_id, const char *event_data);
int ipc_handle_client_message(cli_client_t *client, const char *cmd_str);

/* Helper to format IPC message */
void ipc_format_response(char *buf, size_t buf_size, const char *cmd, const char *fmt, ...);
void ipc_format_event(char *buf, size_t buf_size, const char *event_name, const char *fmt, ...);

#endif /* __HOSTED_DAEMON_IPC_H__ */
