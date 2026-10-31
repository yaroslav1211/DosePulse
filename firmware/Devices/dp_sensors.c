#include "dp_sensors.h"
#include <string.h>
static int32_t signed16(const uint8_t *p){uint32_t u=((uint32_t)p[0]<<8)|p[1];return u&0x8000u?(int32_t)u-65536:(int32_t)u;}
dp_result dp_mpu_start(dp_bus *b,uint8_t address){
 uint8_t id=0;dp_result r=b->read(b->ctx,address,0x75,&id,1);
 if(r!=DP_OK)return r;
 if(id!=0x68)return DP_ID;
 const uint8_t regs[]={0x6b,0x1a,0x1b,0x1c,0x19};
 const uint8_t values[]={0,3,0,0,19};
 for(size_t i=0;i<sizeof(regs);i++){r=b->write(b->ctx,address,regs[i],values[i]);if(r!=DP_OK)return r;}
 return DP_OK;
}
void dp_mpu_decode(const uint8_t raw[14],dp_motion *m){
 memcpy(m->bytes,raw,14);
 for(unsigned i=0;i<3;i++){
  m->accel_mg[i]=signed16(raw+i*2)*1000/16384;
  m->gyro_mdps[i]=signed16(raw+8+i*2)*1000/131;
 }
 m->temp_mc=signed16(raw+6)*1000/340+36530;
}
dp_result dp_mpu_read(dp_bus *b,uint8_t address,dp_motion *m){
 uint8_t bytes[14];dp_result r=b->read(b->ctx,address,0x3b,bytes,sizeof(bytes));
 if(r==DP_OK)dp_mpu_decode(bytes,m);
 return r;
}
dp_result dp_hx_read(dp_bus *b,int32_t *raw){
 if(b->data(b->ctx))return DP_NOT_READY;
 uint32_t value=0;
 for(unsigned i=0;i<24;i++){
  b->clock(b->ctx,1);b->delay_us(b->ctx,2);
  value=(value<<1)|(b->data(b->ctx)?1u:0u);
  b->clock(b->ctx,0);b->delay_us(b->ctx,2);
 }
 b->clock(b->ctx,1);b->delay_us(b->ctx,2);b->clock(b->ctx,0);b->delay_us(b->ctx,2);
 if(value==0x7fffffu||value==0x800000u)return DP_RANGE;
 *raw=value&0x800000u?(int32_t)value-16777216:(int32_t)value;
 return DP_OK;
}
int32_t dp_hx_mg(int32_t raw,int32_t zero){return (int32_t)(((int64_t)raw-zero)*5000000/2100);}
uint32_t dp_target_g(uint16_t raw){return 100u+((uint32_t)raw*900u+2047u)/4095u;}
uint32_t dp_adc_mv(uint16_t raw){return (uint32_t)raw*3300u/4095u;}
