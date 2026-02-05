#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "peripheral_i2c.h"

#define TAG "peripherals_i2c"

i2c_master_bus_handle_t g_i2c_master_bus_handle[I2C_NUM_MAX];
pthread_mutex_t g_i2c_transmit_mutex = PTHREAD_MUTEX_INITIALIZER;

static int do_i2cdetect_cmd(i2c_master_bus_handle_t tool_bus_handle)
{
    uint8_t address;
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
    for (int i = 0; i < 128; i += 16) {
        printf("%02x: ", i);
        for (int j = 0; j < 16; j++) {
            fflush(stdout);
            address = i + j;
            esp_err_t ret = i2c_master_probe(tool_bus_handle, address, I2C_TOOL_TIMEOUT_VALUE_MS);
            if (ret == ESP_OK) {
                printf("%02x ", address);
            } else if (ret == ESP_ERR_TIMEOUT) {
                printf("UU ");
            } else {
                printf("-- ");
            }
        }
        printf("\r\n");
    }

    return 0;
}
esp_err_t i2c_master_init(i2c_port_t i2c_port, gpio_num_t sda, gpio_num_t scl)
{
    esp_err_t err;
    if (i2c_port > I2C_NUM_MAX)
        return -1;

    i2c_master_bus_config_t i2c_mst_config =
    {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = i2c_port,
        .sda_io_num = sda,
        .scl_io_num = scl,

        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    err = i2c_new_master_bus(&i2c_mst_config, &g_i2c_master_bus_handle[i2c_port]);
    if (err != ESP_OK)
        return err;

    do_i2cdetect_cmd(g_i2c_master_bus_handle[i2c_port]);

    return 0;
}

int i2c_master_add(i2c_port_t i2c_port, i2c_master_dev_handle_t *dev_handle, uint16_t device_address, uint32_t hz)
{
    if (i2c_port > I2C_NUM_MAX)
        return -1;

    i2c_device_config_t dev_cfg =
    {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = device_address,
        .scl_speed_hz = hz,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(g_i2c_master_bus_handle[i2c_port], &dev_cfg, dev_handle));

    return 0;
}

int i2c_master_add_lcd(i2c_port_t i2c_port, esp_lcd_panel_io_i2c_config_t *io_config, esp_lcd_panel_io_handle_t *io_handle)
{
    if (i2c_port > I2C_NUM_MAX)
        return -1;

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(g_i2c_master_bus_handle[i2c_port], io_config, io_handle));

    return 0;
}

esp_err_t p_i2c_master_transmit(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size, int xfer_timeout_ms)
{
    esp_err_t ret = 0;
    pthread_mutex_lock(&g_i2c_transmit_mutex);
    ret = i2c_master_transmit(i2c_dev, write_buffer, write_size, xfer_timeout_ms);
    pthread_mutex_unlock(&g_i2c_transmit_mutex);
    return ret;
}

esp_err_t p_i2c_master_receive(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size, int xfer_timeout_ms)
{

    esp_err_t ret = 0;
    pthread_mutex_lock(&g_i2c_transmit_mutex);
    ret = i2c_master_receive(i2c_dev, read_buffer, read_size, xfer_timeout_ms);
    pthread_mutex_unlock(&g_i2c_transmit_mutex);

    return ret;
}

esp_err_t p_i2c_read_register(i2c_master_dev_handle_t i2c_dev, uint8_t reg, uint8_t *read_buffer, size_t read_size, int xfer_timeout_ms)
{
    esp_err_t ret = 0;
    pthread_mutex_lock(&g_i2c_transmit_mutex);

    ret = i2c_master_transmit_receive(i2c_dev, &reg, 1, read_buffer, read_size, xfer_timeout_ms);

    pthread_mutex_unlock(&g_i2c_transmit_mutex);

    return ret;
}

esp_err_t p_i2c_write_register(i2c_master_dev_handle_t i2c_dev, uint8_t reg, uint8_t *read_buffer, size_t read_size, int xfer_timeout_ms)
{
    esp_err_t ret = 0;
    pthread_mutex_lock(&g_i2c_transmit_mutex);

    ret = i2c_master_transmit_receive(i2c_dev, &reg, 1, read_buffer, read_size, xfer_timeout_ms);

    pthread_mutex_unlock(&g_i2c_transmit_mutex);

    return ret;
}

int i2c_master_mutex(int lock)
{
    if (lock)
    {
        pthread_mutex_lock(&g_i2c_transmit_mutex);
    }
    else
    {
        pthread_mutex_unlock(&g_i2c_transmit_mutex);   
    }

    return 0;
}