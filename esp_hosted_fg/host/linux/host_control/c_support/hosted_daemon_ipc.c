/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Espressif Systems Wireless LAN device driver
 *
 * Copyright (C) 2015-2024 Espressif Systems (Shanghai) PTE LTD
 *
 * IPC server implementation for hosted_daemon - provides IPC interface for hosted_cli
 */

#include "hosted_daemon_ipc.h"
#include "test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>
#include <netdb.h>

/* Default response buffer size for command output */
#define CMD_OUTPUT_BUF_SIZE 4096

/* Helper to format IPC response message */
void ipc_format_response(char *buf, size_t buf_size, const char *cmd, const char *fmt, ...)
{
    va_list args;
    int len;

    len = snprintf(buf, buf_size, "OK %s ", cmd);
    if (len > 0 && fmt) {
        va_start(args, fmt);
        vsnprintf(buf + len, buf_size - len, fmt, args);
        va_end(args);
    }
}

/* Helper to format IPC event message */
void ipc_format_event(char *buf, size_t buf_size, const char *event_name, const char *fmt, ...)
{
    va_list args;
    int len;

    len = snprintf(buf, buf_size, "EVENT %s ", event_name);
    if (len > 0 && fmt) {
        va_start(args, fmt);
        vsnprintf(buf + len, buf_size - len, fmt, args);
        va_end(args);
    }
}

/* Forward declaration of command handlers */
static int handle_ipc_command(const char *cmd, char *response, size_t resp_size);

/* Send message to a specific client */
int ipc_send_to_client(cli_client_t *client, uint32_t cmd_id, const char *payload, uint32_t len)
{
    if (!client || client->fd < 0 || !payload) {
        return -1;
    }

    ipc_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.cmd_id = cmd_id;
    msg.length = len > MAX_IPC_PAYLOAD_SIZE ? MAX_IPC_PAYLOAD_SIZE : len;
    memcpy(msg.payload, payload, msg.length);

    ssize_t sent = send(client->fd, &msg, sizeof(msg.cmd_id) + sizeof(msg.length) + msg.length, 0);
    if (sent < 0) {
        return -1;
    }
    return 0;
}

/* Broadcast event to all subscribed clients */
int ipc_broadcast_event(daemon_ipc_ctx_t *ctx, int event_id, const char *event_data)
{
    if (!ctx || !event_data) {
        return -1;
    }

    char payload[MAX_IPC_PAYLOAD_SIZE];
    snprintf(payload, sizeof(payload), "%s", event_data);

    pthread_mutex_lock(&ctx->clients_lock);

    for (int i = 0; i < MAX_CLI_CONNECTIONS; i++) {
        cli_client_t *client = &ctx->clients[i];
        if (client->active && client->fd >= 0) {
            /* Check if client subscribed to this event */
            int subscribed = 0;
            for (int j = 0; j < MAX_EVENT_SUBSCRIPTIONS; j++) {
                if (client->subscribed_events[j] == event_id ||
                    client->subscribed_events[j] == 0) { /* 0 means all events */
                    subscribed = 1;
                    break;
                }
            }
            if (subscribed) {
                ipc_send_to_client(client, IPC_CMD_EVENT, payload, strlen(payload));
            }
        }
    }

    pthread_mutex_unlock(&ctx->clients_lock);
    return 0;
}

/* Parse and execute command from CLI */
int ipc_handle_client_message(cli_client_t *client, const char *cmd_str)
{
    if (!client || !cmd_str) {
        return -1;
    }

    char response[CMD_OUTPUT_BUF_SIZE];
    memset(response, 0, sizeof(response));

    int result = handle_ipc_command(cmd_str, response, sizeof(response));

    /* Send response to client */
    uint32_t cmd_id = (result == 0) ? IPC_CMD_RESPONSE : IPC_CMD_ERROR;
    return ipc_send_to_client(client, cmd_id, response, strlen(response));
}

/* Client reader thread - reads commands from client */
void *ipc_client_reader(void *arg)
{
    cli_client_t *client = (cli_client_t *)arg;
    char buffer[MAX_IPC_PAYLOAD_SIZE + sizeof(ipc_msg_t)];
    int client_fd = client->fd;

    while (client->active) {
        ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) {
                continue;
            }
            break;
        }

        /* Parse message header */
        if (n < (ssize_t)(sizeof(uint32_t) * 2)) {
            continue;
        }

        ipc_msg_t *msg = (ipc_msg_t *)buffer;
        if (msg->length > MAX_IPC_PAYLOAD_SIZE) {
            continue;
        }

        /* Ensure null-terminated command string */
        msg->payload[msg->length] = '\0';

        /* Handle different command types */
        switch (msg->cmd_id) {
            case IPC_CMD_EXECUTE:
                ipc_handle_client_message(client, (char *)msg->payload);
                break;
            case IPC_CMD_PING:
                ipc_send_to_client(client, IPC_CMD_RESPONSE, "PONG", 4);
                break;
            case IPC_CMD_SUBSCRIBE: {
                /* Subscribe to events - parse event IDs from payload */
                int evt_id = atoi((char *)msg->payload);
                for (int i = 0; i < MAX_EVENT_SUBSCRIPTIONS; i++) {
                    if (client->subscribed_events[i] == 0) {
                        client->subscribed_events[i] = evt_id;
                        break;
                    }
                }
                ipc_send_to_client(client, IPC_CMD_RESPONSE, "SUBSCRIBED", 11);
                break;
            }
            case IPC_CMD_UNSUBSCRIBE: {
                /* Unsubscribe from events */
                int evt_id = atoi((char *)msg->payload);
                for (int i = 0; i < MAX_EVENT_SUBSCRIPTIONS; i++) {
                    if (client->subscribed_events[i] == evt_id) {
                        client->subscribed_events[i] = 0;
                    }
                }
                ipc_send_to_client(client, IPC_CMD_RESPONSE, "UNSUBSCRIBED", 13);
                break;
            }
            default:
                break;
        }
    }

    /* Client disconnected */
    close(client_fd);
    client->fd = -1;
    client->active = 0;

    return NULL;
}

/* Accept thread - accepts new client connections */
void *ipc_accept_thread(void *arg)
{
    daemon_ipc_ctx_t *ctx = (daemon_ipc_ctx_t *)arg;

    while (ctx->active) {
        struct sockaddr_un client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(ctx->server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        /* Find available slot for new client */
        pthread_mutex_lock(&ctx->clients_lock);
        cli_client_t *new_client = NULL;
        for (int i = 0; i < MAX_CLI_CONNECTIONS; i++) {
            if (!ctx->clients[i].active) {
                new_client = &ctx->clients[i];
                break;
            }
        }
        pthread_mutex_unlock(&ctx->clients_lock);

        if (!new_client) {
            /* No slots available */
            close(client_fd);
            continue;
        }

        /* Initialize client context */
        memset(new_client, 0, sizeof(cli_client_t));
        new_client->fd = client_fd;
        new_client->active = 1;
        new_client->ctx = ctx;

        /* Subscribe to all events by default */
        for (int i = 0; i < MAX_EVENT_SUBSCRIPTIONS; i++) {
            new_client->subscribed_events[i] = 0; /* 0 means all events */
        }

        /* Create reader thread for this client */
        if (pthread_create(&new_client->reader_thread, NULL, ipc_client_reader, new_client) != 0) {
            close(client_fd);
            new_client->active = 0;
            new_client->fd = -1;
        }
    }

    return NULL;
}

/* Initialize IPC server */
int ipc_server_init(daemon_ipc_ctx_t *ctx)
{
    if (!ctx) {
        return -1;
    }

    memset(ctx, 0, sizeof(daemon_ipc_ctx_t));

    /* Create Unix domain socket */
    ctx->server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (ctx->server_fd < 0) {
        return -1;
    }

    /* Remove existing socket file */
    unlink(IPC_SOCKET_PATH);

    /* Bind socket */
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, IPC_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(ctx->server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(ctx->server_fd);
        return -1;
    }

    /* Set socket permissions - only root can access */
    chmod(IPC_SOCKET_PATH, 0660);

    /* Listen for connections */
    if (listen(ctx->server_fd, MAX_CLI_CONNECTIONS) < 0) {
        close(ctx->server_fd);
        return -1;
    }

    /* Initialize mutex */
    pthread_mutex_init(&ctx->clients_lock, NULL);

    /* Initialize client structures */
    for (int i = 0; i < MAX_CLI_CONNECTIONS; i++) {
        ctx->clients[i].fd = -1;
        ctx->clients[i].active = 0;
    }

    return 0;
}

/* Start IPC server - spawn accept thread */
int ipc_server_start(daemon_ipc_ctx_t *ctx)
{
    if (!ctx) {
        return -1;
    }

    ctx->active = 1;

    /* Create accept thread */
    if (pthread_create(&ctx->accept_thread, NULL, ipc_accept_thread, ctx) != 0) {
        ctx->active = 0;
        return -1;
    }

    return 0;
}

/* Stop IPC server */
void ipc_server_stop(daemon_ipc_ctx_t *ctx)
{
    if (!ctx) {
        return;
    }

    ctx->active = 0;

    /* Close server socket to unblock accept */
    if (ctx->server_fd >= 0) {
        close(ctx->server_fd);
        ctx->server_fd = -1;
    }

    /* Remove socket file */
    unlink(IPC_SOCKET_PATH);

    /* Wait for accept thread */
    if (ctx->accept_thread) {
        pthread_join(ctx->accept_thread, NULL);
    }

    /* Close all client connections */
    pthread_mutex_lock(&ctx->clients_lock);
    for (int i = 0; i < MAX_CLI_CONNECTIONS; i++) {
        if (ctx->clients[i].active) {
            ctx->clients[i].active = 0;
            if (ctx->clients[i].fd >= 0) {
                close(ctx->clients[i].fd);
            }
            if (ctx->clients[i].reader_thread) {
                pthread_join(ctx->clients[i].reader_thread, NULL);
            }
        }
        ctx->clients[i].fd = -1;
    }
    pthread_mutex_unlock(&ctx->clients_lock);

    /* Destroy mutex */
    pthread_mutex_destroy(&ctx->clients_lock);
}

/* Parse key=value argument */
static int get_arg_value(const char *args, const char *key, char *value, size_t value_size)
{
    if (!args || !key || !value) {
        return -1;
    }

    char pattern[64];
    snprintf(pattern, sizeof(pattern), "%s=", key);

    const char *p = strstr(args, pattern);
    if (!p) {
        return -1;
    }

    p += strlen(pattern);
    const char *end = strchr(p, ' ');
    if (!end) {
        end = p + strlen(p);
    }

    size_t len = end - p;
    if (len >= value_size) {
        len = value_size - 1;
    }

    strncpy(value, p, len);
    value[len] = '\0';
    return 0;
}

/* Convert string to bool */
static int str_to_bool(const char *str)
{
    if (!str) return 0;
    return (strcasecmp(str, "true") == 0 ||
            strcasecmp(str, "yes") == 0 ||
            strcasecmp(str, "1") == 0);
}

/* Handle IPC commands - calls test_utils functions */
static int handle_ipc_command(const char *cmd, char *response, size_t resp_size)
{
    if (!cmd || !response) {
        return -1;
    }

    memset(response, 0, resp_size);

    /* Parse command name and arguments */
    char cmd_name[128] = {0};
    char args[1024] = {0};

    const char *space = strchr(cmd, ' ');
    if (space) {
        size_t name_len = space - cmd;
        if (name_len >= sizeof(cmd_name)) {
            name_len = sizeof(cmd_name) - 1;
        }
        strncpy(cmd_name, cmd, name_len);
        strncpy(args, space + 1, sizeof(args) - 1);
    } else {
        strncpy(cmd_name, cmd, sizeof(cmd_name) - 1);
    }

    /* Remove trailing newline if present */
    char *newline = strchr(cmd_name, '\n');
    if (newline) *newline = '\0';

    int result = -1;
    char value[256];

    /* Command handlers - call test_utils functions and capture output */
    if (strcmp(cmd_name, "get_wifi_mode") == 0) {
        result = test_get_wifi_mode();
        snprintf(response, resp_size, "OK get_wifi_mode");
    }
    else if (strcmp(cmd_name, "set_wifi_mode") == 0) {
        if (get_arg_value(args, "mode", value, sizeof(value)) == 0) {
            int mode = -1;
            if (strcmp(value, "station") == 0) mode = WIFI_MODE_STA;
            else if (strcmp(value, "softap") == 0) mode = WIFI_MODE_AP;
            else if (strcmp(value, "station+softap") == 0) mode = WIFI_MODE_APSTA;
            else if (strcmp(value, "none") == 0) mode = WIFI_MODE_NONE;

            if (mode >= 0) {
                result = test_set_wifi_mode(mode);
                snprintf(response, resp_size, "OK set_wifi_mode mode=%s", value);
            } else {
                snprintf(response, resp_size, "FAIL set_wifi_mode invalid_mode");
            }
        } else {
            snprintf(response, resp_size, "FAIL set_wifi_mode missing_mode_arg");
        }
    }
    else if (strcmp(cmd_name, "get_wifi_mac") == 0) {
        if (get_arg_value(args, "interface", value, sizeof(value)) == 0) {
            char mac[64] = {0};
            if (strcmp(value, "station") == 0) {
                result = test_station_mode_get_mac_addr(mac);
            } else if (strcmp(value, "softap") == 0) {
                result = test_softap_mode_get_mac_addr(mac);
            }
            snprintf(response, resp_size, "OK get_wifi_mac interface=%s mac=%s", value, mac);
        } else {
            snprintf(response, resp_size, "FAIL get_wifi_mac missing_interface_arg");
        }
    }
    else if (strcmp(cmd_name, "get_available_ap") == 0) {
        result = test_get_available_wifi();
        snprintf(response, resp_size, "OK get_available_ap");
    }
    else if (strcmp(cmd_name, "get_connected_ap_info") == 0) {
        result = test_station_mode_get_info();
        snprintf(response, resp_size, "OK get_connected_ap_info");
    }
    else if (strcmp(cmd_name, "connect_ap") == 0) {
        char ssid[64] = {0}, password[64] = {0}, bssid[64] = {0};
        char use_wpa3_str[16] = {0}, listen_interval_str[16] = {0}, band_mode_str[16] = {0};

        get_arg_value(args, "ssid", ssid, sizeof(ssid));
        get_arg_value(args, "password", password, sizeof(password));
        get_arg_value(args, "bssid", bssid, sizeof(bssid));
        get_arg_value(args, "use_wpa3", use_wpa3_str, sizeof(use_wpa3_str));
        get_arg_value(args, "listen_interval", listen_interval_str, sizeof(listen_interval_str));
        get_arg_value(args, "band_mode", band_mode_str, sizeof(band_mode_str));

        int use_wpa3 = str_to_bool(use_wpa3_str);
        int listen_interval = atoi(listen_interval_str);
        int band_mode = 0;
        if (strcmp(band_mode_str, "2.4G") == 0) band_mode = 1;
        else if (strcmp(band_mode_str, "5G") == 0) band_mode = 2;
        else if (strcmp(band_mode_str, "auto") == 0) band_mode = 3;

        result = test_station_mode_connect_with_params(
            strlen(ssid) ? ssid : NULL,
            strlen(password) ? password : NULL,
            strlen(bssid) ? bssid : NULL,
            use_wpa3, listen_interval, band_mode);
        snprintf(response, resp_size, "OK connect_ap");
    }
    else if (strcmp(cmd_name, "disconnect_ap") == 0) {
        char reset_dhcp_str[16] = {0};
        get_arg_value(args, "reset_dhcp", reset_dhcp_str, sizeof(reset_dhcp_str));
        int reset_dhcp = str_to_bool(reset_dhcp_str);
        result = test_station_mode_disconnect_with_params(reset_dhcp);
        snprintf(response, resp_size, "OK disconnect_ap");
    }
    else if (strcmp(cmd_name, "start_softap") == 0) {
        char ssid[64] = {0}, password[64] = {0}, channel_str[16] = {0};
        char sec_prot[32] = {0}, max_conn_str[16] = {0}, hide_ssid_str[16] = {0};
        char bw_str[16] = {0}, band_mode_str[16] = {0};

        get_arg_value(args, "ssid", ssid, sizeof(ssid));
        get_arg_value(args, "password", password, sizeof(password));
        get_arg_value(args, "channel", channel_str, sizeof(channel_str));
        get_arg_value(args, "sec_prot", sec_prot, sizeof(sec_prot));
        get_arg_value(args, "max_conn", max_conn_str, sizeof(max_conn_str));
        get_arg_value(args, "hide_ssid", hide_ssid_str, sizeof(hide_ssid_str));
        get_arg_value(args, "bw", bw_str, sizeof(bw_str));
        get_arg_value(args, "band_mode", band_mode_str, sizeof(band_mode_str));

        result = test_softap_mode_start_with_params(
            strlen(ssid) ? ssid : NULL,
            strlen(password) ? password : NULL,
            atoi(channel_str),
            strlen(sec_prot) ? sec_prot : NULL,
            atoi(max_conn_str),
            str_to_bool(hide_ssid_str),
            atoi(bw_str),
            (strcmp(band_mode_str, "2.4G") == 0) ? 1 : (strcmp(band_mode_str, "5G") == 0) ? 2 :
                (strcmp(band_mode_str, "auto") == 0) ? 3 : 0);
        snprintf(response, resp_size, "OK start_softap");
    }
    else if (strcmp(cmd_name, "stop_softap") == 0) {
        result = test_softap_mode_stop();
        snprintf(response, resp_size, "OK stop_softap");
    }
    else if (strcmp(cmd_name, "get_softap_info") == 0) {
        result = test_softap_mode_get_info();
        snprintf(response, resp_size, "OK get_softap_info");
    }
    else if (strcmp(cmd_name, "softap_connected_clients_info") == 0) {
        result = test_softap_mode_connected_clients_info();
        snprintf(response, resp_size, "OK softap_connected_clients_info");
    }
    else if (strcmp(cmd_name, "set_wifi_power_save") == 0) {
        if (get_arg_value(args, "mode", value, sizeof(value)) == 0) {
            int psmode = -1;
            if (strcmp(value, "none") == 0) psmode = 0;
            else if (strcmp(value, "min") == 0) psmode = 1;
            else if (strcmp(value, "max") == 0) psmode = 2;

            if (psmode >= 0) {
                result = test_wifi_set_power_save_mode_with_params(psmode);
                snprintf(response, resp_size, "OK set_wifi_power_save mode=%s", value);
            } else {
                snprintf(response, resp_size, "FAIL set_wifi_power_save invalid_mode");
            }
        } else {
            snprintf(response, resp_size, "FAIL set_wifi_power_save missing_mode_arg");
        }
    }
    else if (strcmp(cmd_name, "get_wifi_power_save") == 0) {
        result = test_get_wifi_power_save_mode();
        snprintf(response, resp_size, "OK get_wifi_power_save");
    }
    else if (strcmp(cmd_name, "set_country_code") == 0) {
        if (get_arg_value(args, "code", value, sizeof(value)) == 0) {
            result = test_set_country_code_with_params(value);
            snprintf(response, resp_size, "OK set_country_code code=%s", value);
        } else {
            snprintf(response, resp_size, "FAIL set_country_code missing_code_arg");
        }
    }
    else if (strcmp(cmd_name, "get_country_code") == 0) {
        result = test_get_country_code();
        snprintf(response, resp_size, "OK get_country_code");
    }
    else if (strcmp(cmd_name, "get_fw_version") == 0) {
        char version[128] = {0};
        result = test_get_fw_version_with_params(version, sizeof(version));
        if (result == SUCCESS) {
            snprintf(response, resp_size, "OK get_fw_version version=%s", version);
        } else {
            snprintf(response, resp_size, "FAIL get_fw_version");
        }
    }
    else if (strcmp(cmd_name, "enable_wifi") == 0) {
        result = test_enable_wifi();
        snprintf(response, resp_size, "OK enable_wifi");
    }
    else if (strcmp(cmd_name, "disable_wifi") == 0) {
        result = test_disable_wifi();
        snprintf(response, resp_size, "OK disable_wifi");
    }
    else if (strcmp(cmd_name, "enable_bt") == 0) {
        result = test_enable_bt();
        snprintf(response, resp_size, "OK enable_bt");
    }
    else if (strcmp(cmd_name, "disable_bt") == 0) {
        result = test_disable_bt();
        snprintf(response, resp_size, "OK disable_bt");
    }
    else if (strcmp(cmd_name, "heartbeat") == 0) {
        char enable_str[16] = {0}, duration_str[16] = {0};
        get_arg_value(args, "enable", enable_str, sizeof(enable_str));
        get_arg_value(args, "duration", duration_str, sizeof(duration_str));

        int enable = str_to_bool(enable_str);
        int duration = atoi(duration_str);
        if (duration <= 0) duration = 30;

        result = test_heartbeat_with_params(enable, duration);
        snprintf(response, resp_size, "OK heartbeat enable=%d duration=%d", enable, duration);
    }
    else if (strcmp(cmd_name, "status") == 0) {
        /* Return daemon status */
        snprintf(response, resp_size, "OK status daemon=running");
        result = SUCCESS;
    }
    else if (strcmp(cmd_name, "help") == 0) {
        snprintf(response, resp_size,
            "OK help Available commands: get_wifi_mode set_wifi_mode get_wifi_mac "
            "get_available_ap connect_ap disconnect_ap start_softap stop_softap "
            "get_softap_info softap_connected_clients_info set_wifi_power_save "
            "get_wifi_power_save set_country_code get_country_code get_fw_version "
            "enable_wifi disable_wifi enable_bt disable_bt heartbeat status help");
        result = SUCCESS;
    }
    else {
        snprintf(response, resp_size, "FAIL unknown_command %s", cmd_name);
        result = -1;
    }

    return result;
}
