#ifndef DP_I2C_GPIO_H
#define DP_I2C_GPIO_H
#include "dp_bus.h"
void dp_i2c_gpio_start(void);
dp_result dp_i2c_gpio_read(void *,uint8_t,uint8_t,uint8_t *,size_t);
dp_result dp_i2c_gpio_write(void *,uint8_t,uint8_t,uint8_t);
#endif
