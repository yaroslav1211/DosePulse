#include "dp_sensors.h"
#include "dp_quality.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "fixtures/mpu_wokwi.h"
typedef struct {dp_result result;uint8_t id;unsigned writes,read_calls,fail_write;uint32_t word;unsigned bit,edges,delays;int notready;} fake;
static dp_result rd(void *ctx,uint8_t addr,uint8_t reg,uint8_t *out,size_t n){
 fake *f=ctx;assert(addr==0x68||addr==0x69);f->read_calls++;
 if(f->result!=DP_OK)return f->result;
 if(reg==0x75){assert(n==1);*out=f->id;}
 else {assert(reg==0x3b&&n==14);memcpy(out,dp_captured_mpu,14);}
 return DP_OK;
}
static dp_result wr(void *ctx,uint8_t addr,uint8_t reg,uint8_t val){
 fake *f=ctx;static const uint8_t r[]={0x6b,0x1a,0x1b,0x1c,0x19},v[]={0,3,0,0,19};
 assert(addr==0x68);assert(f->writes<5);assert(reg==r[f->writes]&&val==v[f->writes]);f->writes++;
 return f->writes==f->fail_write?DP_TIMEOUT:DP_OK;
}
static int data(void *ctx){fake *f=ctx;if(!f->edges)return f->notready;return (f->word>>(23-f->bit++))&1u;}
static void clockpin(void *ctx,int value){fake *f=ctx;assert(value==(f->edges%2==0));f->edges++;}
static void delay(void *ctx,uint32_t us){fake *f=ctx;assert(us==2);f->delays++;}
static void fill(dp_quality *q,uint32_t start,int32_t mg){dp_motion m={.accel_mg={0,0,1000}};for(unsigned i=0;i<14;i++){uint32_t t=start+i*100;dp_quality_motion(q,&m,true,t);dp_quality_mass(q,mg,true,t);}}
int main(void){
 fake f={.id=0x68};dp_bus bus={&f,rd,wr,data,clockpin,delay};dp_motion motion;
 assert(dp_mpu_start(&bus,0x68)==DP_OK&&f.writes==5);
 f=(fake){.id=0x69};assert(dp_mpu_start(&bus,0x68)==DP_ID&&f.writes==0);
 for(dp_result r=DP_NACK;r<=DP_TIMEOUT;r++){f=(fake){.result=r};assert(dp_mpu_start(&bus,0x68)==r);memset(&motion,0x55,sizeof motion);dp_motion old=motion;assert(dp_mpu_read(&bus,0x68,&motion)==r);assert(!memcmp(&old,&motion,sizeof old));}
 for(unsigned fail=1;fail<=5;fail++){f=(fake){.id=0x68,.fail_write=fail};assert(dp_mpu_start(&bus,0x68)==DP_TIMEOUT&&f.writes==fail);}
 f=(fake){0};assert(dp_mpu_read(&bus,0x68,&motion)==DP_OK);assert(motion.accel_mg[2]==1000&&motion.temp_mc==24001);
 uint8_t negative[14]={0xc0,0,0x20,0,0,0,0,0,0xff,0x7d,0,0x83,0,0};dp_mpu_decode(negative,&motion);assert(motion.accel_mg[0]==-1000&&motion.accel_mg[1]==500&&motion.gyro_mdps[0]==-1000&&motion.gyro_mdps[1]==1000&&motion.temp_mc==36530);
 int32_t raw=123;f=(fake){.notready=1};assert(dp_hx_read(&bus,&raw)==DP_NOT_READY&&f.edges==0&&raw==123);
 const uint32_t words[]={0,105,210,2100,0xffffff,0x800001,0x7fffff,0x800000};
 for(unsigned i=0;i<8;i++){f=(fake){.word=words[i]};dp_result r=dp_hx_read(&bus,&raw);assert(f.edges==50&&f.bit==24&&f.delays==50);if(i>=6)assert(r==DP_RANGE);else {assert(r==DP_OK);int32_t expected=words[i]&0x800000?(int32_t)words[i]-16777216:(int32_t)words[i];assert(raw==expected);}}
 assert(dp_hx_mg(210,0)==500000&&dp_hx_mg(0,210)==-500000&&dp_hx_mg(105,105)==0);
 assert(dp_target_g(0)==100&&dp_target_g(4095)==1000&&dp_target_g(2048)==550&&dp_adc_mv(4095)==3300);
 dp_quality q={0};assert(!dp_quality_stable(&q,0));fill(&q,10,500000);assert(dp_quality_stable(&q,1310));assert(!dp_quality_stable(&q,1700));
 q=(dp_quality){0};fill(&q,0xfffffa00,100000);assert(dp_quality_stable(&q,0xfffffa00u+1300u));
 dp_motion m={.accel_mg={0,0,1000}};q=(dp_quality){0};fill(&q,10,0);m.accel_mg[0]=151;dp_quality_motion(&q,&m,true,1320);assert(!dp_quality_stable(&q,1320)&&q.count==0);
 m.accel_mg[0]=150;dp_quality_motion(&q,&m,true,1340);assert(!q.moving);m.gyro_mdps[1]=-10001;dp_quality_motion(&q,&m,true,1360);assert(q.moving);
 dp_quality_motion(&q,&m,false,1380);assert(!q.motion_valid);dp_quality_mass(&q,100,false,1390);assert(!q.mass_valid&&q.count==0);
 q=(dp_quality){0};m=(dp_motion){.accel_mg={0,0,1000}};for(unsigned i=0;i<20;i++){dp_quality_motion(&q,&m,true,i*100);dp_quality_mass(&q,(int32_t)i*10000,true,i*100);}assert(!dp_quality_stable(&q,1900));
 q=(dp_quality){0};fill(&q,10,10000);dp_quality_motion(&q,&m,true,1410);dp_quality_mass(&q,900000,true,1410);assert(q.filtered_mg==10000);dp_quality_motion(&q,&m,true,1510);dp_quality_mass(&q,900000,true,1510);assert(q.filtered_mg==900000&&!dp_quality_stable(&q,1510));
 q=(dp_quality){0};assert(dp_quality_target(&q,2048)==2048);assert(dp_quality_target(&q,4095)==2559);assert(dp_quality_target(&q,0)==1920);
 puts("PASS DosePulse sensor conversion, bus errors, HX clocks, median, stability, freshness, rollover, target filter");
 return 0;
}
