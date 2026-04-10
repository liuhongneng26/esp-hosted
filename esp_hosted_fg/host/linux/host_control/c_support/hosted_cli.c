/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Espressif Systems Wireless LAN device driver
 *
 * Copyright (C) 2015-2024 Espressif Systems (Shanghai) PTE LTD
 *
 * CLI client for hosted_daemon - communicates with daemon via Unix socket
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <sys/time.h>

#define IPC_SOCKET_PATH "/var/run/hosted.sock"
#define MAX_IPC_PAYLOAD_SIZE 4096

/* IPC Command IDs */
#define IPC_CMD_EXECUTE       0x01
#define IPC_CMD_RESPONSE      0x02
#define IPC_CMD_EVENT        0x03
#define IPC_CMD_ERROR        0x04
#define IPC_CMD_PING          0x07

/* IPC Message */
typedef struct {
    uint32_t cmd_id;
    uint32_t length;
    uint8_t payload[MAX_IPC_PAYLOAD_SIZE];
} ipc_msg_t;

static int cli_fd = -1;
static int interactive_mode = 0;
static pthread_t event_thread;
static volatile int keep_running = 1;

/* Event callback type */
typedef void (*event_callback_t)(const char *event);
static event_callback_t event_callback = NULL;

/* Signal handler for graceful exit */
static void signal_handler(int signum)
{
    (void)signum;
    keep_running = 0;
    if (cli_fd >= 0) {
        close(cli_fd);
        cli_fd = -1;
    }
}

/* Connect to daemon IPC socket */
static int cli_connect(void)
{
    cli_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (cli_fd < 0) {
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, IPC_SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(cli_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(cli_fd);
        cli_fd = -1;
        return -1;
    }

    return 0;
}

/* Disconnect from daemon */
static void cli_disconnect(void)
{
    if (cli_fd >= 0) {
        close(cli_fd);
        cli_fd = -1;
    }
}

/* Send command and receive response synchronously */
static int cli_send_command(const char *cmd, char *response, size_t resp_size)
{
    if (!cmd || !response || cli_fd < 0) {
        return -1;
    }

    /* Send command */
    ipc_msg_t send_msg;
    memset(&send_msg, 0, sizeof(send_msg));
    send_msg.cmd_id = IPC_CMD_EXECUTE;
    size_t cmd_len = strlen(cmd);
    if (cmd_len > MAX_IPC_PAYLOAD_SIZE) {
        cmd_len = MAX_IPC_PAYLOAD_SIZE;
    }
    send_msg.length = cmd_len;
    memcpy(send_msg.payload, cmd, cmd_len);

    ssize_t sent = send(cli_fd, &send_msg, sizeof(send_msg.cmd_id) + sizeof(send_msg.length) + send_msg.length, 0);
    if (sent < 0) {
        return -1;
    }

    /* Receive response */
    ipc_msg_t recv_msg;
    memset(&recv_msg, 0, sizeof(recv_msg));

    ssize_t n = recv(cli_fd, &recv_msg, sizeof(recv_msg), 0);
    if (n <= 0) {
        return -1;
    }

    /* Parse response header */
    if (recv_msg.length >= MAX_IPC_PAYLOAD_SIZE) {
        recv_msg.length = MAX_IPC_PAYLOAD_SIZE - 1;
    }
    recv_msg.payload[recv_msg.length] = '\0';

    /* Check if response or error */
    if (recv_msg.cmd_id == IPC_CMD_ERROR) {
        snprintf(response, resp_size, "ERROR: %s", recv_msg.payload);
        return -1;
    }

    /* Copy response payload */
    snprintf(response, resp_size, "%s", recv_msg.payload);
    return 0;
}

/* Event receiver thread */
static void *event_receiver_thread(void *arg)
{
    (void)arg;

    while (keep_running) {
        fd_set rfds;
        struct timeval tv;

        FD_ZERO(&rfds);
        FD_SET(cli_fd, &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int ret = select(cli_fd + 1, &rfds, NULL, NULL, &tv);
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (ret == 0) continue;

        ipc_msg_t msg;
        memset(&msg, 0, sizeof(msg));

        ssize_t n = recv(cli_fd, &msg, sizeof(msg), 0);
        if (n <= 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (msg.cmd_id == IPC_CMD_EVENT) {
            if (msg.length >= MAX_IPC_PAYLOAD_SIZE) {
                msg.length = MAX_IPC_PAYLOAD_SIZE - 1;
            }
            msg.payload[msg.length] = '\0';

            if (event_callback) {
                event_callback((char *)msg.payload);
            } else if (interactive_mode) {
                printf("\n[EVENT] %s\nesp-hosted> ", msg.payload);
                fflush(stdout);
            }
        }
    }

    return NULL;
}

/* Print help */
static void print_help(void)
{
    printf("ESP-Hosted CLI - Available commands:\n");
    printf("  get_wifi_mode                    - Get current Wi-Fi mode\n");
    printf("  set_wifi_mode mode=<mode>        - Set Wi-Fi mode (station/softap/station+softap/none)\n");
    printf("  get_wifi_mac interface=<iface>   - Get MAC address (station/softap)\n");
    printf("  get_available_ap                 - Scan for available APs\n");
    printf("  get_connected_ap_info           - Get info of connected AP\n");
    printf("  connect_ap ssid=<ssid> password=<pwd> [options] - Connect to AP\n");
    printf("    Options: bssid=<bssid> use_wpa3=<0|1> listen_interval=<n> band_mode=<2.4G|5G|auto>\n");
    printf("  disconnect_ap [reset_dhcp=<0|1>] - Disconnect from AP\n");
    printf("  start_softap ssid=<ssid> password=<pwd> [options] - Start SoftAP\n");
    printf("    Options: channel=<n> sec_prot=<open|wpa_psk|wpa2_psk|wpa_wpa2_psk>\n");
    printf("             max_conn=<n> hide_ssid=<0|1> bw=<20|40> band_mode=<2.4G|5G|auto>\n");
    printf("  stop_softap                      - Stop SoftAP\n");
    printf("  get_softap_info                  - Get SoftAP configuration\n");
    printf("  softap_connected_clients_info    - Get connected stations to SoftAP\n");
    printf("  set_wifi_power_save mode=<mode>  - Set power save (none/min/max)\n");
    printf("  get_wifi_power_save              - Get power save mode\n");
    printf("  set_country_code code=<code>     - Set country code (e.g. US, CN)\n");
    printf("  get_country_code                 - Get country code\n");
    printf("  get_fw_version                   - Get firmware version\n");
    printf("  enable_wifi                      - Enable Wi-Fi\n");
    printf("  disable_wifi                     - Disable Wi-Fi\n");
    printf("  enable_bt                        - Enable Bluetooth\n");
    printf("  disable_bt                       - Disable Bluetooth\n");
    printf("  heartbeat [enable=<0|1>] [duration=<sec>] - Configure heartbeat\n");
    printf("  status                           - Get daemon status\n");
    printf("  help                             - Show this help\n");
    printf("  quit                             - Exit CLI\n");
}

/* Run single command mode */
static int run_single_command(const char *cmd)
{
    if (cli_connect() < 0) {
        fprintf(stderr, "Failed to connect to daemon. Is hosted_daemon running?\n");
        return 1;
    }

    char response[MAX_IPC_PAYLOAD_SIZE * 2];
    int result = cli_send_command(cmd, response, sizeof(response));

    if (result == 0) {
        printf("%s\n", response);
    } else {
        printf("%s\n", response);
    }

    cli_disconnect();
    return result;
}

/* Run interactive mode */
static void run_interactive(void)
{
    char line[1024];

    if (cli_connect() < 0) {
        fprintf(stderr, "Failed to connect to daemon. Is hosted_daemon running?\n");
        return;
    }

    /* Start event receiver thread */
    keep_running = 1;
    if (pthread_create(&event_thread, NULL, event_receiver_thread, NULL) != 0) {
        fprintf(stderr, "Failed to create event thread\n");
    }

    printf("ESP-Hosted CLI (connected to daemon)\n");
    printf("Type 'help' for available commands, 'quit' to exit\n\n");

    while (keep_running) {
        printf("esp-hosted> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }

        /* Remove newline */
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }

        /* Skip empty lines */
        if (len == 0) {
            continue;
        }

        /* Handle built-in commands */
        if (strcmp(line, "quit") == 0 || strcmp(line, "exit") == 0) {
            break;
        }

        if (strcmp(line, "help") == 0) {
            print_help();
            continue;
        }

        /* Send command to daemon */
        char response[MAX_IPC_PAYLOAD_SIZE * 2];
        int result = cli_send_command(line, response, sizeof(response));

        if (result == 0) {
            printf("%s\n", response);
        } else {
            printf("%s\n", response);
        }
    }

    keep_running = 0;

    /* Wait for event thread */
    if (event_thread) {
        pthread_join(event_thread, NULL);
    }

    cli_disconnect();
}

int main(int argc, char *argv[])
{
    /* Check for root privileges */
    if (getuid() != 0) {
        fprintf(stderr, "This program must be run as root\n");
        return 1;
    }

    /* Set up signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Parse arguments */
    if (argc > 1) {
        if (strcmp(argv[1], "-i") == 0 || strcmp(argv[1], "--interactive") == 0) {
            interactive_mode = 1;
            run_interactive();
        } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help();
        } else {
            /* Single command mode - concatenate all args */
            char cmd[4096] = {0};
            for (int i = 1; i < argc; i++) {
                if (i > 1) {
                    strncat(cmd, " ", sizeof(cmd) - strlen(cmd) - 1);
                }
                strncat(cmd, argv[i], sizeof(cmd) - strlen(cmd) - 1);
            }
            return run_single_command(cmd);
        }
    } else {
        /* Default to interactive mode */
        interactive_mode = 1;
        run_interactive();
    }

    return 0;
}
