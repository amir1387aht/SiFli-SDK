#include "aht20.h"
#include "sensor.h"
#include <rtthread.h>
#include <rtdbg.h>

static rt_size_t aht20_fetch_data(struct rt_sensor_device *sensor, void *buf, rt_size_t len)
{
    struct rt_sensor_data *data = (struct rt_sensor_data *)buf;
    float temp = 0, humi = 0;

    if (aht20_startmeasure() != 0)
    {
        LOG_E("AHT20 start measure failed");
        return 0;
    }
    if (aht20_getmeasureresult(&temp, &humi) != 0)
    {
        LOG_E("AHT20 get result failed");
        return 0;
    }

    data->type = RT_SENSOR_CLASS_TEMP;
    data->data.temp = (rt_int32_t)(temp * 10);
    data->timestamp = rt_sensor_get_ts();

    data++;
    data->type = RT_SENSOR_CLASS_HUMI;
    data->data.humi = (rt_int32_t)(humi * 10);
    data->timestamp = rt_sensor_get_ts();

    return 2;
}

static rt_err_t aht20_control(struct rt_sensor_device *sensor, int cmd, void *args)
{
    return RT_EOK;
}

static struct rt_sensor_ops sensor_ops =
{
    aht20_fetch_data,
    aht20_control
};

int rt_hw_aht20_init(const char *name, struct rt_sensor_config *cfg)
{
    int result = -RT_ERROR;
    rt_sensor_t sensor = RT_NULL;

    if (aht20_calibrate() != 0)
    {
        LOG_E("AHT20 calibrate failed");
        return -RT_ERROR;
    }

    sensor = rt_calloc(1, sizeof(struct rt_sensor_device));
    if (RT_NULL == sensor)
    {
        LOG_E("calloc failed");
        return -RT_ERROR;
    }

    sensor->info.type       = RT_SENSOR_CLASS_TEMP;
    sensor->info.vendor     = RT_SENSOR_VENDOR_UNKNOWN;
    sensor->info.model      = "aht20";
    sensor->info.unit       = RT_SENSOR_UNIT_DCELSIUS;
    sensor->info.intf_type  = RT_SENSOR_INTF_I2C;
    sensor->info.range_max  = 850;
    sensor->info.range_min  = -400;
    sensor->info.period_min = 10;

    rt_memcpy(&sensor->config, cfg, sizeof(struct rt_sensor_config));
    sensor->ops = &sensor_ops;

    result = rt_hw_sensor_register(sensor, name, RT_DEVICE_FLAG_RDWR, RT_NULL);
    if (result != RT_EOK)
    {
        LOG_E("device register err code: %d", result);
        rt_free(sensor);
        return -RT_ERROR;
    }
    else
    {
        LOG_I("AHT20 sensor init success");
        return RT_EOK;
    }
}

static int rt_aht20_auto_register(void)
{
    struct rt_sensor_config cfg = {0};

    /* Set the sensor configuration parameters */
    cfg.intf.dev_name = AHT20_I2C_BUS;    /* Modification according to the actual I2C equipment used */
    cfg.intf.user_data = (void *)0x38;

    return rt_hw_aht20_init("aht20", &cfg);
}
INIT_APP_EXPORT(rt_aht20_auto_register);