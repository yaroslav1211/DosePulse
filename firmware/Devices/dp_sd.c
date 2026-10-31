/* Raw block bring-up only. Product journal belongs to Lab3. SDHC SPI mode0. */
#include "dp_sd.h"
static void end(dp_spi *s){s->select(s->ctx,0);s->exchange(s->ctx,0xff);}
static uint8_t command(dp_spi *s,uint8_t cmd,uint32_t arg){
 end(s);s->select(s->ctx,1);s->exchange(s->ctx,0xff);
 s->exchange(s->ctx,0x40|cmd);
 for(int shift=24;shift>=0;shift-=8)s->exchange(s->ctx,(uint8_t)(arg>>shift));
 s->exchange(s->ctx,cmd==0?0x95:cmd==8?0x87:0x01);
 for(unsigned i=0;i<16;i++){uint8_t r=s->exchange(s->ctx,0xff);if(!(r&0x80))return r;}
 return 0xff;
}
dp_result dp_sd_start(dp_spi *s){
 end(s);s->delay_ms(s->ctx,2);for(unsigned i=0;i<10;i++)s->exchange(s->ctx,0xff);
 if(command(s,0,0)!=1){end(s);return DP_TIMEOUT;}
 if(command(s,8,0x1aa)!=1){end(s);return DP_ID;}
 uint32_t echo=0;for(unsigned i=0;i<4;i++)echo=(echo<<8)|s->exchange(s->ctx,0xff);
 if(echo!=0x1aa){end(s);return DP_ID;}
 unsigned tries;
 for(tries=0;tries<100;tries++){
  if(command(s,55,0)>1){end(s);return DP_ID;}
  uint8_t r=command(s,41,0x40000000);
  if(!r)break;
  if(r!=1){end(s);return DP_ID;}
  s->delay_ms(s->ctx,10);
 }
 if(tries==100){end(s);return DP_TIMEOUT;}
 if(command(s,58,0)!=0){end(s);return DP_ID;}
 uint32_t ocr=0;for(unsigned i=0;i<4;i++)ocr=(ocr<<8)|s->exchange(s->ctx,0xff);
 end(s);return (ocr&0xc0000000)==0xc0000000?DP_OK:DP_ID;
}
dp_result dp_sd_read(dp_spi *s,uint32_t sector,uint8_t data[512]){
 if(command(s,17,sector)!=0){end(s);return DP_NACK;}
 uint8_t token=0xff;unsigned i;
 for(i=0;i<10000;i++){token=s->exchange(s->ctx,0xff);if(token!=0xff)break;}
 if(token!=0xfe){end(s);return DP_TIMEOUT;}
 for(i=0;i<512;i++)data[i]=s->exchange(s->ctx,0xff);
 s->exchange(s->ctx,0xff);s->exchange(s->ctx,0xff);end(s);return DP_OK;
}
dp_result dp_sd_write(dp_spi *s,uint32_t sector,const uint8_t data[512]){
 if(command(s,24,sector)!=0){end(s);return DP_NACK;}
 s->exchange(s->ctx,0xff);s->exchange(s->ctx,0xfe);
 for(unsigned i=0;i<512;i++)s->exchange(s->ctx,data[i]);
 s->exchange(s->ctx,0xff);s->exchange(s->ctx,0xff);
 uint8_t response=s->exchange(s->ctx,0xff);
 if((response&0x1f)!=5){end(s);return DP_NACK;}
 for(unsigned i=0;i<20000;i++)if(s->exchange(s->ctx,0xff)==0xff){end(s);return DP_OK;}
 end(s);return DP_TIMEOUT;
}
