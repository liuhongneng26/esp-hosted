#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "sensor.h"

#define TAG "BMP280"

#define BMP280_I2C_ADDRESS  0x77 // 77 I2C address when SDO pin is high; 0x76 low

/**
 * BMP280 registers
 */
#define BMP280_REG_TEMP_XLSB   0xFC /* bits: 7-4 */
#define BMP280_REG_TEMP_LSB    0xFB
#define BMP280_REG_TEMP_MSB    0xFA
#define BMP280_REG_TEMP        (BMP280_REG_TEMP_MSB)
#define BMP280_REG_PRESS_XLSB  0xF9 /* bits: 7-4 */
#define BMP280_REG_PRESS_LSB   0xF8
#define BMP280_REG_PRESS_MSB   0xF7
#define BMP280_REG_PRESSURE    (BMP280_REG_PRESS_MSB)
#define BMP280_REG_CONFIG      0xF5 /* bits: 7-5 t_sb; 4-2 filter; 0 spi3w_en */
#define BMP280_REG_CTRL        0xF4 /* bits: 7-5 osrs_t; 4-2 osrs_p; 1-0 mode */
#define BMP280_REG_STATUS      0xF3 /* bits: 3 measuring; 0 im_update */
#define BMP280_REG_CTRL_HUM    0xF2 /* bits: 2-0 osrs_h; */
#define BMP280_REG_RESET       0xE0
#define BMP280_REG_ID          0xD0
#define BMP280_REG_CALIB       0x88
#define BMP280_REG_HUM_CALIB   0x88

#define BMP280_RESET_VALUE     0xB6

typedef struct
{
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;

    /* Humidity compensation for BME280 */
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;

    uint8_t   id;       //!< Chip ID
} bmp280_t;

typedef enum
{
    BMP280_STANDBY_05 = 0,      //!< stand by time 0.5ms
    BMP280_STANDBY_62 = 1,      //!< stand by time 62.5ms
    BMP280_STANDBY_125 = 2,     //!< stand by time 125ms
    BMP280_STANDBY_250 = 3,     //!< stand by time 250ms
    BMP280_STANDBY_500 = 4,     //!< stand by time 500ms
    BMP280_STANDBY_1000 = 5,    //!< stand by time 1s
    BMP280_STANDBY_2000 = 6,    //!< stand by time 2s BMP280, 10ms BME280
    BMP280_STANDBY_4000 = 7,    //!< stand by time 4s BMP280, 20ms BME280
} BMP280_StandbyTime;

typedef enum
{
    BMP280_FILTER_OFF = 0,
    BMP280_FILTER_2 = 1,
    BMP280_FILTER_4 = 2,
    BMP280_FILTER_8 = 3,
    BMP280_FILTER_16 = 4
} BMP280_Filter;

typedef enum
{
    BMP280_SKIPPED = 0,          //!< no measurement
    BMP280_ULTRA_LOW_POWER = 1,  //!< oversampling x1
    BMP280_LOW_POWER = 2,        //!< oversampling x2
    BMP280_STANDARD = 3,         //!< oversampling x4
    BMP280_HIGH_RES = 4,         //!< oversampling x8
    BMP280_ULTRA_HIGH_RES = 5    //!< oversampling x16
} BMP280_Oversampling;
typedef enum
{
    BMP280_MODE_SLEEP = 0,  //!< Sleep mode
    BMP280_MODE_FORCED = 1, //!< Measurement is initiated by user
    BMP280_MODE_NORMAL = 3  //!< Continues measurement
} BMP280_Mode;

bmp280_t g_bmp280;
static i2c_master_dev_handle_t dev_handle;


static esp_err_t read_register16(uint8_t reg, uint16_t *r)
{
    uint8_t d[] = { 0, 0 };

    p_i2c_read_register(dev_handle, reg, d, 2, -1);
    *r = d[0] | (d[1] << 8);

    return ESP_OK;
}

static esp_err_t read_calibration_data(bmp280_t *dev)
{
    read_register16(0x88, &dev->dig_T1);
    read_register16( 0x8a, (uint16_t *)&dev->dig_T2);
    read_register16( 0x8c, (uint16_t *)&dev->dig_T3);
    read_register16( 0x8e, &dev->dig_P1);
    read_register16( 0x90, (uint16_t *)&dev->dig_P2);
    read_register16( 0x92, (uint16_t *)&dev->dig_P3);
    read_register16( 0x94, (uint16_t *)&dev->dig_P4);
    read_register16( 0x96, (uint16_t *)&dev->dig_P5);
    read_register16( 0x98, (uint16_t *)&dev->dig_P6);
    read_register16( 0x9a, (uint16_t *)&dev->dig_P7);
    read_register16( 0x9c, (uint16_t *)&dev->dig_P8);
    read_register16( 0x9e, (uint16_t *)&dev->dig_P9);

    ESP_LOGD(TAG, "Calibration data received:");
    ESP_LOGD(TAG, "dig_T1=%d", dev->dig_T1);
    ESP_LOGD(TAG, "dig_T2=%d", dev->dig_T2);
    ESP_LOGD(TAG, "dig_T3=%d", dev->dig_T3);
    ESP_LOGD(TAG, "dig_P1=%d", dev->dig_P1);
    ESP_LOGD(TAG, "dig_P2=%d", dev->dig_P2);
    ESP_LOGD(TAG, "dig_P3=%d", dev->dig_P3);
    ESP_LOGD(TAG, "dig_P4=%d", dev->dig_P4);
    ESP_LOGD(TAG, "dig_P5=%d", dev->dig_P5);
    ESP_LOGD(TAG, "dig_P6=%d", dev->dig_P6);
    ESP_LOGD(TAG, "dig_P7=%d", dev->dig_P7);
    ESP_LOGD(TAG, "dig_P8=%d", dev->dig_P8);
    ESP_LOGD(TAG, "dig_P9=%d", dev->dig_P9);

    return ESP_OK;
}

static inline int32_t compensate_temperature(bmp280_t *dev, int32_t adc_temp, int32_t *fine_temp)
{
    int32_t var1, var2;

    var1 = ((((adc_temp >> 3) - ((int32_t)dev->dig_T1 << 1))) * (int32_t)dev->dig_T2) >> 11;
    var2 = (((((adc_temp >> 4) - (int32_t)dev->dig_T1) * ((adc_temp >> 4) - (int32_t)dev->dig_T1)) >> 12) * (int32_t)dev->dig_T3) >> 14;

    *fine_temp = var1 + var2;
    return (*fine_temp * 5 + 128) >> 8;
}

static inline uint32_t compensate_pressure(bmp280_t *dev, int32_t adc_press, int32_t fine_temp)
{
    int64_t var1, var2, p;

    var1 = (int64_t)fine_temp - 128000;
    var2 = var1 * var1 * (int64_t)dev->dig_P6;
    var2 = var2 + ((var1 * (int64_t)dev->dig_P5) << 17);
    var2 = var2 + (((int64_t)dev->dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dev->dig_P3) >> 8) + ((var1 * (int64_t)dev->dig_P2) << 12);
    var1 = (((int64_t)1 << 47) + var1) * ((int64_t)dev->dig_P1) >> 33;

    if (var1 == 0)
    {
        return 0;  // avoid exception caused by division by zero
    }

    p = 1048576 - adc_press;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = ((int64_t)dev->dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((int64_t)dev->dig_P8 * p) >> 19;

    p = ((p + var1 + var2) >> 8) + ((int64_t)dev->dig_P7 << 4);
    return p;
}

int bmp280_int(i2c_port_t i2c_port)
{
    int ret;
    uint8_t buf[8];

    memset(&g_bmp280, 0, sizeof(bmp280_t));

    ret = i2c_master_add(i2c_port, &dev_handle, BMP280_I2C_ADDRESS, 100000);
    if (ret != 0)
    {
        printf("bmp280 init error!\n");
        
        return -1;
    }

    p_i2c_read_register(dev_handle, BMP280_REG_ID, &g_bmp280.id, 1, -1);
    printf("bmp280 chip id %x\n", g_bmp280.id);

    buf[0] = BMP280_REG_RESET;
    buf[1] = BMP280_RESET_VALUE;
    p_i2c_master_transmit(dev_handle, buf, 2, -1);

    while (1)
    {
        uint8_t status;
        if (!p_i2c_read_register(dev_handle, BMP280_REG_STATUS, &status, 1, -1) && (status & 1) == 0)
            break;
    }

    read_calibration_data(&g_bmp280);

    buf[0] = BMP280_REG_CONFIG;
    buf[1] = (BMP280_STANDBY_250 << 5) | (BMP280_FILTER_OFF << 2);
    p_i2c_master_transmit(dev_handle, buf, 2, -1);



    buf[0] = BMP280_REG_CTRL;
    buf[1] = (BMP280_STANDARD << 5) | (BMP280_STANDARD << 2) | BMP280_MODE_NORMAL;
    p_i2c_master_transmit(dev_handle, buf, 2, -1);

    return 0;
}

int bmp280_read(float *temperature, float *pressure, float *humidity)
{ 
    int32_t adc_pressure = 0;
    int32_t adc_temp = 0;
    int32_t fine_temp;

    if (dev_handle == NULL || temperature == NULL || pressure == NULL)
        return -1;

    uint8_t data[8];
    int size = 6;

    p_i2c_read_register(dev_handle, 0xf7, data, size, -1);

    adc_pressure = data[0] << 12 | data[1] << 4 | data[2] >> 4;
    adc_temp = data[3] << 12 | data[4] << 4 | data[5] >> 4;

    *temperature = compensate_temperature(&g_bmp280, adc_temp, &fine_temp);
    *pressure = compensate_pressure(&g_bmp280, adc_pressure, fine_temp);

    *temperature = (float)*temperature / 100;
    *pressure = (float)*pressure / 256;
    return 0;
}