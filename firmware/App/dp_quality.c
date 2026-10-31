#include "dp_quality.h"
#include <stdlib.h>
static void clear(dp_quality *q){q->count=0;q->index=0;q->accumulating=false;q->median_count=0;q->median_index=0;}
void dp_quality_motion(dp_quality *q,const dp_motion *m,bool valid,uint32_t now){
 q->motion_valid=valid;q->last_motion=now;
 q->moving=!valid;
 if(valid){
  q->moving=abs(m->accel_mg[0])+abs(m->accel_mg[1])+abs(m->accel_mg[2]-1000)>150;
  for(unsigned i=0;i<3;i++)if(abs(m->gyro_mdps[i])>10000)q->moving=true;
 }
 if(q->moving)clear(q);
}
void dp_quality_mass(dp_quality *q,int32_t mg,bool valid,uint32_t now){
 q->mass_valid=valid;q->last_mass=now;
 if(!valid||!q->motion_valid||q->moving||(uint32_t)(now-q->last_motion)>100){clear(q);return;}
 q->median[q->median_index]=mg;q->median_index=(q->median_index+1)%3;
 if(q->median_count<3)q->median_count++;
 if(q->median_count<3){q->filtered_mg=mg;return;}
 int32_t a=q->median[0],b=q->median[1],c=q->median[2],t;
 if(a>b){t=a;a=b;b=t;}if(b>c){t=b;b=c;c=t;}if(a>b){t=a;a=b;b=t;}
 q->filtered_mg=b;q->window[q->index]=b;q->index=(q->index+1)%10;
 if(q->count<10)q->count++;
 int32_t lo=b,hi=b;
 for(unsigned i=0;i<q->count;i++){if(q->window[i]<lo)lo=q->window[i];if(q->window[i]>hi)hi=q->window[i];}
 if(hi-lo>10000){q->since=now;q->accumulating=false;return;}
 if(!q->accumulating){q->since=now;q->accumulating=true;}
}
bool dp_quality_stable(dp_quality *q,uint32_t now){
 if((uint32_t)(now-q->last_mass)>350)q->mass_valid=false;
 if((uint32_t)(now-q->last_motion)>100)q->motion_valid=false;
 if(!q->mass_valid||!q->motion_valid||q->moving||(uint32_t)(now-q->last_mass)>350||(uint32_t)(now-q->last_motion)>100){clear(q);return false;}
 return q->accumulating&&q->count==10&&(uint32_t)(now-q->since)>=1000;
}
uint16_t dp_quality_target(dp_quality *q,uint16_t raw){
 if(!q->adc_seen){q->adc_q2=(uint32_t)raw*4;q->adc_seen=true;}
 else q->adc_q2=q->adc_q2-q->adc_q2/4+raw;
 return (uint16_t)(q->adc_q2/4);
}
