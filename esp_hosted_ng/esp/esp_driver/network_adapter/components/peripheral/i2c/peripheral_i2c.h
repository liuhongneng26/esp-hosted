#ifndef __PERIPHERAL_I2C_H__
#define __PERIPHERAL_I2C_H__

#include "esp_err.h"
#include "soc/gpio_num.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include <stdint.h>

#define I2C_TOOL_TIMEOUT_VALUE_MS (50)

struct i2c_target
{
    char *name;
    i2c_port_t port;
    gpio_num_t sda;
    gpio_num_t scl;
};

esp_err_t i2c_master_init(i2c_port_t i2c_port, gpio_num_t sda, gpio_num_t scl);

int i2c_master_add(i2c_port_t i2c_port, i2c_master_dev_handle_t *dev_handle, uint16_t device_address, uint32_t hz);
int i2c_master_add_lcd(i2c_port_t i2c_port, esp_lcd_panel_io_i2c_config_t *io_config, esp_lcd_panel_io_handle_t *io_handle);

esp_err_t p_i2c_master_transmit(i2c_master_dev_handle_t i2c_dev, const uint8_t *write_buffer, size_t write_size, int xfer_timeout_ms);

esp_err_t p_i2c_master_receive(i2c_master_dev_handle_t i2c_dev, uint8_t *read_buffer, size_t read_size, int xfer_timeout_ms);

esp_err_t p_i2c_read_register(i2c_master_dev_handle_t i2c_dev, uint8_t reg, uint8_t *read_buffer, size_t read_size, int xfer_timeout_ms);

int i2c_master_mutex(int lock);

#endif