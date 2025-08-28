#ifndef AHT20_H
#define AHT20_H

#include <rtthread.h>
#include <rtdevice.h>
#include <stdint.h>

uint32_t aht20_calibrate(void);
uint32_t aht20_startmeasure(void);
uint32_t aht20_getmeasureresult(float *temp, float *humi);
rt_err_t aht20_init(const char *i2c_bus_name);
rt_err_t aht20_measure(float *temp, float *humi);
#endif  // AHT20_H