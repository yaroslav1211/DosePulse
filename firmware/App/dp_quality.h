#ifndef DP_QUALITY_H
#define DP_QUALITY_H
#include <stdint.h>
#include <stdbool.h>
#include "dp_sensors.h"
typedef struct {
 int32_t median[3],window[10],filtered_mg;
 unsigned median_count,median_index,count,index;
 uint32_t since,last_mass,last_motion;bool accumulating,moving,mass_valid,motion_valid;
 uint32_t adc_q2;bool adc_seen;
} dp_quality;
void dp_quality_motion(dp_quality *q,const dp_motion *m,bool valid,uint32_t now);
void dp_quality_mass(dp_quality *q,int32_t mg,bool valid,uint32_t now);
bool dp_quality_stable(dp_quality *q,uint32_t now);
uint16_t dp_quality_target(dp_quality *q,uint16_t raw);
#endif
