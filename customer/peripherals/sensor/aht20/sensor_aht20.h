#ifndef SENSOR_AHT20_H
#define SENSOR_AHT20_H
#include "aht20.h"
#include "sensor.h"
#include <rtthread.h>
#include <rtdbg.h>

int rt_hw_aht20_init(const char *name, struct rt_sensor_config *cfg);

#endif
