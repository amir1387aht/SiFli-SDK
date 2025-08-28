#include "aht20.h"
#include <rtthread.h>
#include <string.h>
#include <stdio.h>

#define AHT20_ADDR         0x38

#define AHT20_CMD_CALIBRATION       0xBE
#define AHT20_CMD_CALIBRATION_ARG0  0x08
#define AHT20_CMD_CALIBRATION_ARG1  0x00

#define AHT20_CMD_TRIGGER       0xAC
#define AHT20_CMD_TRIGGER_ARG0  0x33
#define AHT20_CMD_TRIGGER_ARG1  0x00

#define AHT20_CMD_RESET      0xBA
#define AHT20_CMD_STATUS     0x71

#define AHT20_STATUS_BUSY_SHIFT 7
#define AHT20_STATUS_BUSY_MASK  (0x1<<AHT20_STATUS_BUSY_SHIFT)
#define AHT20_STATUS_BUSY(status) ((status & AHT20_STATUS_BUSY_MASK) >> AHT20_STATUS_BUSY_SHIFT)

#define AHT20_STATUS_CALI_SHIFT 3
#define AHT20_STATUS_CALI_MASK  (0x1<<AHT20_STATUS_CALI_SHIFT)
#define AHT20_STATUS_CALI(status) ((status & AHT20_STATUS_CALI_MASK) >> AHT20_STATUS_CALI_SHIFT)

#define AHT20_STATUS_RESPONSE_MAX 6
#define AHT20_RESOLUTION            (1<<20)
#define AHT20_MAX_RETRY 10

#define AHT20_STARTUP_TIME     20   // ms
#define AHT20_CALIBRATION_TIME 40   // ms
#define AHT20_MEASURE_TIME     75   // ms
static struct rt_i2c_bus_device *aht20_i2c_bus = RT_NULL;

static rt_err_t aht20_write_cmd(uint8_t *cmd, rt_size_t len)
{
    struct rt_i2c_msg msgs;
    msgs.addr  = AHT20_ADDR;
    msgs.flags = RT_I2C_WR;
    msgs.buf   = cmd;
    msgs.len   = len;
    return rt_i2c_transfer(aht20_i2c_bus, &msgs, 1) == 1 ? RT_EOK : -RT_ERROR;
}

static rt_err_t aht20_read_bytes(uint8_t *buf, rt_size_t len)
{
    struct rt_i2c_msg msgs;
    msgs.addr  = AHT20_ADDR;
    msgs.flags = RT_I2C_RD;
    msgs.buf   = buf;
    msgs.len   = len;
    return rt_i2c_transfer(aht20_i2c_bus, &msgs, 1) == 1 ? RT_EOK : -RT_ERROR;
}

// Send get status command
static rt_err_t aht20_status_command(void)
{
    uint8_t statusCmd[] = { AHT20_CMD_STATUS };
    return aht20_write_cmd(statusCmd, sizeof(statusCmd));
}

// Send soft reset command
static rt_err_t aht20_reset_command(void)
{
    uint8_t resetCmd[] = {AHT20_CMD_RESET};
    return aht20_write_cmd(resetCmd, sizeof(resetCmd));
}

// Send the initialization calibration command
static rt_err_t aht20_calibrate_command(void)
{
    uint8_t clibrateCmd[] = {AHT20_CMD_CALIBRATION, AHT20_CMD_CALIBRATION_ARG0, AHT20_CMD_CALIBRATION_ARG1};
    return aht20_write_cmd(clibrateCmd, sizeof(clibrateCmd));
}

rt_err_t aht20_init(const char *i2c_bus_name)
{
    aht20_i2c_bus = (struct rt_i2c_bus_device *)rt_i2c_bus_device_find(i2c_bus_name);
    if (!aht20_i2c_bus)
    {
        rt_kprintf("AHT20: can't find i2c bus %s\n", i2c_bus_name);
        return -RT_ERROR;
    }

    uint8_t buffer[AHT20_STATUS_RESPONSE_MAX];
    memset(buffer, 0x0, sizeof(buffer));

    if (aht20_status_command() != RT_EOK)
        return -RT_ERROR;

    if (aht20_read_bytes(buffer, 1) != RT_EOK)
        return -RT_ERROR;

    if (AHT20_STATUS_BUSY(buffer[0]) || !AHT20_STATUS_CALI(buffer[0]))
    {
        if (aht20_reset_command() != RT_EOK)
            return -RT_ERROR;
        rt_thread_mdelay(AHT20_STARTUP_TIME);

        if (aht20_calibrate_command() != RT_EOK)
            return -RT_ERROR;
        rt_thread_mdelay(AHT20_CALIBRATION_TIME);
    }

    return RT_EOK;
}

rt_err_t aht20_start_measure(void)
{
    uint8_t triggerCmd[] = {AHT20_CMD_TRIGGER, AHT20_CMD_TRIGGER_ARG0, AHT20_CMD_TRIGGER_ARG1};
    return aht20_write_cmd(triggerCmd, sizeof(triggerCmd));
}

rt_err_t aht20_measure(float *temp, float *humi)
{
    if (aht20_start_measure() != RT_EOK)
        return -RT_ERROR;
    rt_thread_mdelay(AHT20_MEASURE_TIME);

    uint8_t buffer[AHT20_STATUS_RESPONSE_MAX];
    memset(buffer, 0x0, sizeof(buffer));
    if (aht20_read_bytes(buffer, sizeof(buffer)) != RT_EOK)
        return -RT_ERROR;

    int i = 0;
    while (AHT20_STATUS_BUSY(buffer[0]) && i < AHT20_MAX_RETRY)
    {
        rt_thread_mdelay(AHT20_MEASURE_TIME);
        if (aht20_read_bytes(buffer, sizeof(buffer)) != RT_EOK)
            return -RT_ERROR;
        i++;
    }
    if (i >= AHT20_MAX_RETRY)
    {
        rt_kprintf("AHT20 device always busy!\n");
        return -RT_ERROR;
    }

    uint32_t humiRaw = buffer[1];
    humiRaw = (humiRaw << 8) | buffer[2];
    humiRaw = (humiRaw << 4) | ((buffer[3] & 0xF0) >> 4);
    *humi = humiRaw / (float)AHT20_RESOLUTION * 100;

    uint32_t tempRaw = buffer[3] & 0x0F;
    tempRaw = (tempRaw << 8) | buffer[4];
    tempRaw = (tempRaw << 8) | buffer[5];
    *temp = tempRaw / (float)AHT20_RESOLUTION * 200 - 50;

    return RT_EOK;
}


uint32_t aht20_calibrate(void)
{
    uint8_t buffer[AHT20_STATUS_RESPONSE_MAX];
    memset(buffer, 0x0, sizeof(buffer));

    uint8_t statusCmd = AHT20_CMD_STATUS;
    if (aht20_write_cmd(&statusCmd, 1) != RT_EOK)
        return (uint32_t) -1;
    if (aht20_read_bytes(buffer, 1) != RT_EOK)
        return (uint32_t) -1;

    if (AHT20_STATUS_BUSY(buffer[0]) || !AHT20_STATUS_CALI(buffer[0]))
    {
        uint8_t resetCmd = AHT20_CMD_RESET;
        if (aht20_write_cmd(&resetCmd, 1) != RT_EOK)
            return (uint32_t) -1;
        rt_thread_mdelay(AHT20_STARTUP_TIME);

        uint8_t calibCmd[3] = {AHT20_CMD_CALIBRATION, AHT20_CMD_CALIBRATION_ARG0, AHT20_CMD_CALIBRATION_ARG1};
        if (aht20_write_cmd(calibCmd, 3) != RT_EOK)
            return (uint32_t) -1;
        rt_thread_mdelay(AHT20_CALIBRATION_TIME);
    }

    return 0;
}

uint32_t aht20_startmeasure(void)
{
    uint8_t triggerCmd[3] = {AHT20_CMD_TRIGGER, AHT20_CMD_TRIGGER_ARG0, AHT20_CMD_TRIGGER_ARG1};
    if (aht20_write_cmd(triggerCmd, 3) != RT_EOK)
        return (uint32_t) -1;
    return 0;
}

uint32_t aht20_getmeasureresult(float *temp, float *humi)
{
    if (temp == NULL || humi == NULL)
        return (uint32_t) -1;

    uint8_t buffer[AHT20_STATUS_RESPONSE_MAX];
    memset(buffer, 0x0, sizeof(buffer));
    if (aht20_read_bytes(buffer, sizeof(buffer)) != RT_EOK)
        return (uint32_t) -1;

    int i = 0;
    while (AHT20_STATUS_BUSY(buffer[0]) && i < AHT20_MAX_RETRY)
    {
        rt_thread_mdelay(AHT20_MEASURE_TIME);
        if (aht20_read_bytes(buffer, sizeof(buffer)) != RT_EOK)
            return (uint32_t) -1;
        i++;
    }
    if (i >= AHT20_MAX_RETRY)
    {
        rt_kprintf("AHT20 device always busy!\n");
        return (uint32_t) -1;
    }

    uint32_t humiRaw = buffer[1];
    humiRaw = (humiRaw << 8) | buffer[2];
    humiRaw = (humiRaw << 4) | ((buffer[3] & 0xF0) >> 4);
    *humi = humiRaw / (float)AHT20_RESOLUTION * 100;

    uint32_t tempRaw = buffer[3] & 0x0F;
    tempRaw = (tempRaw << 8) | buffer[4];
    tempRaw = (tempRaw << 8) | buffer[5];
    *temp = tempRaw / (float)AHT20_RESOLUTION * 200 - 50;

    return 0;
}