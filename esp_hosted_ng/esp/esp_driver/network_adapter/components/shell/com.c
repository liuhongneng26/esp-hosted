/* UART Select Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart_vfs.h"
#include "driver/uart.h"
#include "esp_console.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_log.h"
#include "argtable3/argtable3.h"
#include "hal/i2c_types.h"
#include "sdkconfig.h"
#include <stdio.h>

static const char* TAG = "uart_select_example";
int picocom_main (int argc, char *argv[]);

#define STI STDIN_FILENO
#define STI_RD_SZ 16

static void uart_select_task(void *arg)
{
    uart_vfs_dev_register();
    if (uart_driver_install(UART_NUM_0, 2 * 1024, 0, 0, NULL, 0) != ESP_OK) {
        ESP_LOGE(TAG, "Driver installation failed");
        // return ;
    }

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_param_config(UART_NUM_0, &uart_config);
    
    uart_vfs_dev_use_driver(UART_NUM_0);

    uart_vfs_dev_port_set_rx_line_endings(UART_NUM_0, ESP_LINE_ENDINGS_CR);
    uart_vfs_dev_port_set_tx_line_endings(UART_NUM_0, ESP_LINE_ENDINGS_CRLF);

    int fd;
    int i, n;

    if ((fd = open("/dev/uart/0", O_RDWR)) == -1)
    {
        ESP_LOGE(TAG, "Cannot open UART");
        return ;
    }
    while (1) 
    {
        int s;
        fd_set rfds;

        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        FD_SET(STDIN_FILENO, &rfds);

        s = select(fd + 1, &rfds, NULL, NULL, NULL);

        if (s < 0)
        {
            ESP_LOGE(TAG, "Select failed: errno %d (%s)", errno, strerror(errno));
            break;
        }
        else if (s == 0)
        {
            ESP_LOGI(TAG, "Timeout has been reached and nothing has been received");
        }

        if (FD_ISSET(fd, &rfds))
        {
            char buf[32];
            memset(buf, 0, 32);
            n = read(fd, &buf, 32);
            if (n > 0)
            {
            for ( i = 0; i < n; i++ )
            {
                if (buf[i] == '\r')
                    continue;
                printf("%c", buf[i]);
            }
            }
            else
            {
                ESP_LOGE(TAG, "UART read error");
                break;
            }
        }

        if (FD_ISSET(STDIN_FILENO, &rfds))
        {
            char buff_rd[STI_RD_SZ];
            unsigned char c;
            do
            {
                n = read(STI, buff_rd, sizeof(buff_rd));
            }while (n < 0 && errno == EINTR);
            
            for ( i = 0; i < n; i++ )
            {
                c = buff_rd[i];
                if (c == '\r')
                    continue;
                write(fd, &c, 1);
            }
        }

    }

    close(fd);

    return ;
}

static int do_com_cmd(int argc, char **argv)
{
    uart_select_task(NULL);

    // picocom_main(argc, argv);

    return 0;
}

void register_com(void)
{
    const esp_console_cmd_t i2cdetect_cmd = {
        .command = "com",
        .help = "serial uart",
        .hint = NULL,
        .func = &do_com_cmd,
        .argtable = NULL
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&i2cdetect_cmd));
}