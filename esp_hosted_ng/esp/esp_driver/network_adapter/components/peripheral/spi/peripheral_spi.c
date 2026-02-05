#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"


#include "peripheral_spi.h"

#define TAG "peripherals_spi"

int spi_master_init(spi_host_device_t host_id, int miso, int mosi, int clk)
{
    esp_err_t ret = 0;
    spi_bus_config_t spi_bus_conf = {0};

    /* SPI总线配置 */
    spi_bus_conf.miso_io_num = miso;                               /* SPI_MISO引脚 */
    spi_bus_conf.mosi_io_num = mosi;                               /* SPI_MOSI引脚 */
    spi_bus_conf.sclk_io_num = clk;                                /* SPI_SCLK引脚 */
    spi_bus_conf.quadwp_io_num = -1;                                            /* SPI写保护信号引脚，该引脚未使能 */
    spi_bus_conf.quadhd_io_num = -1;                                            /* SPI保持信号引脚，该引脚未使能 */
    spi_bus_conf.max_transfer_sz = 160 * 80 * 2;                                /* 配置最大传输大小，以字节为单位 */
    
    /* 初始化SPI总线 */
    ret = spi_bus_initialize(host_id, &spi_bus_conf, SPI_DMA_CH_AUTO);        /* SPI总线初始化 */
    return ret;
}

int spi_master_add(spi_host_device_t host_id, spi_device_handle_t *handle, int cs, int clock_speed_hz)
{
    esp_err_t ret = 0;
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = clock_speed_hz,                         /* SPI时钟 */
        .mode = 0,                                                  /* SPI模式0 */
        .spics_io_num = cs,                                /* SPI设备引脚 */
        .queue_size = 7,                                            /* 事务队列尺寸 7个 */
    };
    
    /* 添加SPI总线设备 */
    ret = spi_bus_add_device(host_id, &devcfg, handle);   /* 配置SPI总线设备 */
    ESP_ERROR_CHECK(ret);
    return ret;
}

void spi_master_write_cmd(spi_device_handle_t handle, uint8_t cmd)
{
    esp_err_t ret;
    spi_transaction_t t = {0};

    t.length = 8;                                       /* 要传输的位数 一个字节 8位 */
    t.tx_buffer = &cmd;                                 /* 将命令填充进去 */
    ret = spi_device_polling_transmit(handle, &t);      /* 开始传输 */
    ESP_ERROR_CHECK(ret);                               /* 一般不会有问题 */
}

void spi_master_write_data(spi_device_handle_t handle, const uint8_t *data, int len)
{
    esp_err_t ret;
    spi_transaction_t t = {0};

    if (len == 0)
    {
        return;                                     /* 长度为0 没有数据要传输 */
    }

    t.length = len * 8;                             /* 要传输的位数 一个字节 8位 */
    t.tx_buffer = data;                             /* 将命令填充进去 */
    ret = spi_device_polling_transmit(handle, &t);  /* 开始传输 */
    ESP_ERROR_CHECK(ret);                           /* 一般不会有问题 */
}

uint8_t spi_master_transfer_byte(spi_device_handle_t handle, uint8_t data)
{
    spi_transaction_t t;

    memset(&t, 0, sizeof(t));

    t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    t.length = 8;
    t.tx_data[0] = data;
    spi_device_transmit(handle, &t);

    return t.rx_data[0];
}