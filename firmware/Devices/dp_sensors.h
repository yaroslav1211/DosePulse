#ifndef DP_SENSORS_H
#define DP_SENSORS_H
#include "dp_bus.h"
typedef struct {int32_t accel_mg[3],gyro_mdps[3];int32_t temp_mc;uint8_t bytes[14];} dp_motion;
dp_result dp_mpu_start(dp_bus *b,uint8_t address);
dp_result dp_mpu_read(dp_bus *b,uint8_t address,dp_motion *m);
void dp_mpu_decode(const uint8_t raw[14],dp_motion *m);
dp_result dp_hx_read(dp_bus *b,int32_t *raw);
int32_t dp_hx_mg(int32_t raw,int32_t zero);
uint32_t dp_target_g(uint16_t raw);
uint32_t dp_adc_mv(uint16_t raw);
#endif
