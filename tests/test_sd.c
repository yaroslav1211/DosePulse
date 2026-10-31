#include "dp_sd.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {
 uint8_t response[1024],packet[6],disk[512];
 unsigned read,write,bytes,phase,index,commands[256],count,acmd;
 int selected,mode;
} fake_sd;
static void push(fake_sd *f,uint8_t b){assert(f->write<sizeof(f->response));f->response[f->write++]=b;}
static void select_card(void *ctx,int selected){fake_sd *f=ctx;f->selected=selected;if(!selected){f->bytes=0;f->phase=0;f->read=f->write=0;}}
static void delay(void *ctx,unsigned ms){(void)ctx;assert(ms==2||ms==10);}
static uint8_t exchange(void *ctx,uint8_t value){
 fake_sd *f=ctx;
 if(!f->selected)return 0xff;
 if(f->read<f->write)return f->response[f->read++];
 if(f->mode==14&&f->phase==4)return 0;
 if(f->phase==1){if(value==0xfe){f->phase=2;f->index=0;}else assert(value==0xff);return 0xff;}
 if(f->phase==2){f->disk[f->index++]=value;if(f->index==512){f->phase=3;f->index=0;}return 0xff;}
 if(f->phase==3){assert(value==0xff);if(++f->index==2){push(f,f->mode==13?0x0b:5);push(f,0);if(f->mode!=14)push(f,0xff);f->phase=4;}return 0xff;}
 if(!f->bytes&&(value<0x40||value>0x7f))return 0xff;
 f->packet[f->bytes++]=value;
 if(f->bytes==6){
  unsigned cmd=f->packet[0]&0x3f;assert(f->count<256);f->commands[f->count++]=cmd;f->bytes=0;
  assert(f->packet[5]==(cmd==0?0x95:cmd==8?0x87:1));
  f->read=f->write=0;
  if(cmd==0){if(f->mode!=1)push(f,1);}
  else if(cmd==8){push(f,f->mode==2?5:1);push(f,0);push(f,0);push(f,1);push(f,f->mode==3?0xab:0xaa);}
  else if(cmd==55)push(f,f->mode==4?5:1);
  else if(cmd==41)push(f,f->mode==5?5:f->mode==6?1:f->acmd++==0?1:0);
  else if(cmd==58){push(f,f->mode==7?5:0);push(f,f->mode==8?0x80:0xc0);push(f,0xff);push(f,0x80);push(f,0);}
  else if(cmd==17){assert(f->packet[1]==0&&f->packet[2]==0&&f->packet[3]==0x10&&f->packet[4]==0);push(f,f->mode==9?5:0);push(f,f->mode==10?0x0d:0xfe);for(unsigned i=0;i<512;i++)push(f,f->disk[i]);push(f,0xff);push(f,0xff);}
  else if(cmd==24){push(f,f->mode==12?5:0);f->phase=1;}
  else assert(!"Unexpected command");
 }
 return 0xff;
}
int main(void){
 fake_sd f={0};dp_spi spi={&f,exchange,select_card,delay};assert(dp_sd_start(&spi)==DP_OK);
 const unsigned expected[]={0,8,55,41,55,41,58};assert(f.count==7&&!memcmp(f.commands,expected,sizeof expected));
 uint8_t input[512],output[512];for(unsigned i=0;i<512;i++)input[i]=(uint8_t)(i*37+19);
 assert(dp_sd_write(&spi,4096,input)==DP_OK);assert(dp_sd_read(&spi,4096,output)==DP_OK);assert(!memcmp(input,output,512));
 for(int mode=1;mode<=8;mode++){f=(fake_sd){.mode=mode};dp_result r=dp_sd_start(&spi);assert(r==(mode==1||mode==6?DP_TIMEOUT:DP_ID));}
 f=(fake_sd){.mode=9};assert(dp_sd_read(&spi,4096,output)==DP_NACK);
 f=(fake_sd){.mode=10};assert(dp_sd_read(&spi,4096,output)==DP_TIMEOUT);
 f=(fake_sd){.mode=12};assert(dp_sd_write(&spi,4096,input)==DP_NACK);
 f=(fake_sd){.mode=13};assert(dp_sd_write(&spi,4096,input)==DP_NACK);
 f=(fake_sd){.mode=14};assert(dp_sd_write(&spi,4096,input)==DP_TIMEOUT);
 puts("PASS SD command/CRC order, SDHC identity, raw read/write, data token, busy timeout, failures");
}
