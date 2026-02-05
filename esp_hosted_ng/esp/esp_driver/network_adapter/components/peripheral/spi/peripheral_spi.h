#ifndef __PERIPHERAL_SPI_H__
#define __PERIPHERAL_SPI_H__

#include "driver/spi_master.h"
#include "soc/gpio_num.h"

struct spi_target
{
    char *name;
    spi_host_device_t port;
    gpio_num_t mosi;
    gpio_num_t miso;
    gpio_num_t clk;
    int max_transfer_sz;
};

int spi_master_init(spi_host_device_t host_id, int miso, int mosi, int clk);
int spi_master_add(spi_host_device_t i2c_port, spi_device_handle_t *handle, int cs, int clock_speed_hz);

void spi_master_write_cmd(spi_device_handle_t handle, uint8_t cmd);
void spi_master_write_data(spi_device_handle_t handle, const uint8_t *data, int len);
uint8_t spi_master_transfer_byte(spi_device_handle_t handle, uint8_t data);
#endif