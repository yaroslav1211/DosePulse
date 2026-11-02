/* Board knowledge and the Lab2 integration bench. No portion state machine. */
#include "main.h"
#include "dp_bench.h"
#include "dp_sensors.h"
#include "dp_quality.h"
#include "dp_sd.h"
#include "dp_i2c_gpio.h"
#include <stdio.h>
#include <string.h>
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;
static ADC_HandleTypeDef adc;
static dp_quality quality;
static dp_motion motion;
static uint8_t address=0x68;
static bool mpu_ready,hx_off,adc_valid;
static dp_result imu_result,hx_result;
static int32_t hx_raw,tare_mg;
static uint16_t adc_raw,adc_filtered;
static uint32_t last_imu,last_hx,last_adc,last_report,retry_at;
static uint32_t imu_count,hx_count,adc_count,bus_errors;
static volatile uint32_t rx_overflow;
static volatile bool rx_lost;
static uint8_t rx_byte,rx_ring[128];
static volatile unsigned rx_head,rx_tail;
static char command_line[64];
static unsigned command_size;
static bool discard;
static void wait_us(void *ctx,uint32_t us){
 (void)ctx;uint16_t start=(uint16_t)TIM3->CNT;
 while((uint16_t)((uint16_t)TIM3->CNT-start)<us){}
}
static dp_result translate(HAL_StatusTypeDef status){
 if(status==HAL_OK)return DP_OK;
 if(status==HAL_TIMEOUT)return DP_TIMEOUT;
 return DP_NACK;
}
static int hx_data(void *ctx){(void)ctx;return HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_1)==GPIO_PIN_SET;}
static void hx_clock(void *ctx,int high){(void)ctx;HAL_GPIO_WritePin(GPIOA,GPIO_PIN_8,high?GPIO_PIN_SET:GPIO_PIN_RESET);}
static dp_bus bus={0,dp_i2c_gpio_read,dp_i2c_gpio_write,hx_data,hx_clock,wait_us};
static uint8_t spi_exchange(void *ctx,uint8_t value){
 (void)ctx;uint8_t result=0;
 for(unsigned i=0;i<8;i++){
  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5,(value&0x80)?GPIO_PIN_SET:GPIO_PIN_RESET);value<<=1;
  wait_us(0,2);HAL_GPIO_WritePin(GPIOB,GPIO_PIN_3,GPIO_PIN_SET);wait_us(0,2);
  result=(uint8_t)((result<<1)|(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_4)==GPIO_PIN_SET?1:0));
  HAL_GPIO_WritePin(GPIOB,GPIO_PIN_3,GPIO_PIN_RESET);
 }
 return result;
}
static void spi_select(void *ctx,int selected){(void)ctx;HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,selected?GPIO_PIN_RESET:GPIO_PIN_SET);}
static void delay_ms(void *ctx,unsigned ms){(void)ctx;HAL_Delay(ms);}
static dp_spi sd={0,spi_exchange,spi_select,delay_ms};
static uint8_t sector[512];
static uint32_t checksum(const uint8_t *p,size_t n){uint32_t c=2166136261u;for(size_t i=0;i<n;i++)c=(c^p[i])*16777619u;return c;}
static void storage_probe(bool write){
 dp_result r=dp_sd_start(&sd);uint32_t before=0,after=0;
 if(r==DP_OK)r=dp_sd_read(&sd,4096,sector);
 if(r==DP_OK)before=checksum(sector,512);
 if(write&&r==DP_OK){
  for(unsigned i=0;i<512;i++)sector[i]=(uint8_t)(i*37u+19u);
  r=dp_sd_write(&sd,4096,sector);
  if(r==DP_OK)r=dp_sd_read(&sd,4096,sector);
  if(r==DP_OK)for(unsigned i=0;i<512;i++)if(sector[i]!=(uint8_t)(i*37u+19u)){r=DP_RANGE;break;}
 }
 if(r==DP_OK)after=checksum(sector,512);
 printf("SD sector=4096 write=%u result=%u before=%08lx after=%08lx\r\n",write,(unsigned)r,(unsigned long)before,(unsigned long)after);
}
static dp_result read_adc(void){
 HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_SET);
 HAL_StatusTypeDef r=HAL_ADC_Start(&adc);
 if(r==HAL_OK)r=HAL_ADC_PollForConversion(&adc,3);
 if(r==HAL_OK)adc_raw=(uint16_t)HAL_ADC_GetValue(&adc);
 HAL_ADC_Stop(&adc);HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_RESET);
 return translate(r);
}
static void status(void){
 uint32_t now=HAL_GetTick();bool stable=dp_quality_stable(&quality,now);
 printf("DATA t=%lu hx=%ld mg=%ld net_mg=%ld hx_ok=%u imu=%u a_mg=%ld,%ld,%ld gyro_mdps=%ld,%ld,%ld temp_mc=%ld adc=%u mv=%lu target_g=%lu moving=%u stable=%u\r\n",
 (unsigned long)now,(long)hx_raw,(long)quality.filtered_mg,(long)(quality.filtered_mg-tare_mg),quality.mass_valid,(unsigned)imu_result,
 (long)motion.accel_mg[0],(long)motion.accel_mg[1],(long)motion.accel_mg[2],(long)motion.gyro_mdps[0],(long)motion.gyro_mdps[1],(long)motion.gyro_mdps[2],(long)motion.temp_mc,adc_raw,(unsigned long)dp_adc_mv(adc_raw),(unsigned long)dp_target_g(adc_filtered),quality.moving,stable);
 HAL_GPIO_WritePin(GPIOA,GPIO_PIN_5,stable?GPIO_PIN_SET:GPIO_PIN_RESET);
}
static void selftest(void){
 dp_result r=dp_mpu_start(&bus,address);mpu_ready=r==DP_OK;
 printf("SELF device=MPU6050 addr=%02x result=%u expected_id=68 state=%u err=%lx isr=%lx\r\n",address,(unsigned)r,(unsigned)HAL_I2C_GetState(&hi2c1),(unsigned long)HAL_I2C_GetError(&hi2c1),(unsigned long)I2C1->ISR);
 printf("SELF device=HX711 id=none result=%u raw=%ld valid=%u\r\n",(unsigned)hx_result,(long)hx_raw,quality.mass_valid);
 printf("SELF device=ADC result=%u raw=%u mv=%lu\r\n",adc_valid?0:2,adc_raw,(unsigned long)dp_adc_mv(adc_raw));
}
static void command(const char *line){
 if(!strcmp(line,"status")||!strcmp(line,"read"))status();
 else if(!strcmp(line,"selftest"))selftest();
 else if(!strcmp(line,"anomaly")){
  uint8_t id=0;dp_result r=bus.read(0,0x69,0x75,&id,1);
  printf("ANOMALY addr=69 reg=75 result=%u hardware_diag_error=%lx\r\n",(unsigned)r,(unsigned long)HAL_I2C_GetError(&hi2c1));
 }
 else if(!strcmp(line,"fault")){address=0x69;mpu_ready=false;dp_quality_motion(&quality,&motion,false,HAL_GetTick());puts("ACK fault addr=69");}
 else if(!strcmp(line,"recover")){address=0x68;mpu_ready=false;puts("ACK recover addr=68");}
 else if(!strcmp(line,"hxoff")){hx_off=true;hx_clock(0,1);dp_quality_mass(&quality,0,false,HAL_GetTick());puts("ACK HX711 powerdown");}
 else if(!strcmp(line,"hxon")){hx_clock(0,0);hx_off=false;puts("ACK HX711 powerup");}
 else if(!strcmp(line,"tare")){
  if(dp_quality_stable(&quality,HAL_GetTick())){tare_mg=quality.filtered_mg;printf("ACK tare_mg=%ld\r\n",(long)tare_mg);}else puts("ERR unstable");
 }
 else if(!strcmp(line,"sdprobe"))storage_probe(true);
 else if(!strcmp(line,"sdread"))storage_probe(false);
 else if(!strcmp(line,"counts"))printf("COUNTS imu=%lu hx=%lu adc=%lu errors=%lu rx_overflow=%lu\r\n",(unsigned long)imu_count,(unsigned long)hx_count,(unsigned long)adc_count,(unsigned long)bus_errors,(unsigned long)rx_overflow);
 else puts("ERR command");
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart){
 if(uart!=&huart2)return;
 unsigned next=(rx_head+1)%sizeof(rx_ring);
 if(next==rx_tail){rx_overflow++;rx_lost=true;}else{rx_ring[rx_head]=rx_byte;rx_head=next;}
 HAL_UART_Receive_IT(&huart2,&rx_byte,1);
}
void USART2_IRQHandler(void){HAL_UART_IRQHandler(&huart2);}
void dp_bench_start(void){
 __HAL_RCC_TIM3_CLK_ENABLE();TIM3->PSC=47;TIM3->ARR=65535;TIM3->EGR=TIM_EGR_UG;TIM3->CR1=TIM_CR1_CEN;
 GPIO_InitTypeDef g={0};g.Mode=GPIO_MODE_OUTPUT_PP;g.Speed=GPIO_SPEED_FREQ_HIGH;
 g.Pin=GPIO_PIN_8|GPIO_PIN_4;HAL_GPIO_Init(GPIOA,&g);HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);hx_clock(0,0);
 g.Pin=GPIO_PIN_0|GPIO_PIN_3|GPIO_PIN_5;HAL_GPIO_Init(GPIOB,&g);
 g.Mode=GPIO_MODE_INPUT;g.Pull=GPIO_PULLUP;g.Pin=GPIO_PIN_1;HAL_GPIO_Init(GPIOA,&g);g.Pin=GPIO_PIN_4;HAL_GPIO_Init(GPIOB,&g);
 g.Mode=GPIO_MODE_ANALOG;g.Pull=GPIO_NOPULL;g.Pin=GPIO_PIN_0;HAL_GPIO_Init(GPIOA,&g);__HAL_RCC_ADC_CLK_ENABLE();
 adc.Instance=ADC1;adc.Init.ClockPrescaler=ADC_CLOCK_SYNC_PCLK_DIV4;adc.Init.Resolution=ADC_RESOLUTION_12B;
 adc.Init.DataAlign=ADC_DATAALIGN_RIGHT;adc.Init.ScanConvMode=ADC_SCAN_DISABLE;adc.Init.EOCSelection=ADC_EOC_SINGLE_CONV;
 adc.Init.NbrOfConversion=1;adc.Init.ExternalTrigConv=ADC_SOFTWARE_START;adc.Init.ExternalTrigConvEdge=ADC_EXTERNALTRIGCONVEDGE_NONE;
 adc.Init.Overrun=ADC_OVR_DATA_PRESERVED;adc.Init.SamplingTimeCommon1=ADC_SAMPLETIME_79CYCLES_5;
 if(HAL_ADC_Init(&adc)!=HAL_OK)Error_Handler();
 if(HAL_ADCEx_Calibration_Start(&adc)!=HAL_OK)Error_Handler();
 ADC_ChannelConfTypeDef ch={0};ch.Channel=ADC_CHANNEL_0;ch.Rank=ADC_REGULAR_RANK_1;ch.SamplingTime=ADC_SAMPLINGTIME_COMMON_1;
 if(HAL_ADC_ConfigChannel(&adc,&ch)!=HAL_OK)Error_Handler();
 adc_valid=read_adc()==DP_OK;adc_filtered=dp_quality_target(&quality,adc_raw);
 HAL_NVIC_SetPriority(USART2_IRQn,1,0);HAL_NVIC_EnableIRQ(USART2_IRQn);HAL_UART_Receive_IT(&huart2,&rx_byte,1);
 dp_i2c_gpio_start();
 puts("DosePulse lab2 sensor-bench schema=1 HAL-GPIO-I2C no-portion-engine");
 hx_result=DP_NOT_READY;selftest();
}
void dp_bench_poll(void){
 uint32_t now=HAL_GetTick();
 if((uint32_t)(now-last_imu)>=20){
  last_imu=now;
  if(!mpu_ready&&(int32_t)(now-retry_at)>=0){imu_result=dp_mpu_start(&bus,address);mpu_ready=imu_result==DP_OK;retry_at=now+500;}
  imu_result=mpu_ready?dp_mpu_read(&bus,address,&motion):DP_NACK;
  if(imu_result!=DP_OK){mpu_ready=false;bus_errors++;}
  dp_quality_motion(&quality,&motion,imu_result==DP_OK,now);imu_count++;
 }
 if((uint32_t)(now-last_hx)>=10&&!hx_off){
  last_hx=now;uint32_t mask=__get_PRIMASK();__disable_irq();hx_result=dp_hx_read(&bus,&hx_raw);__set_PRIMASK(mask);
  if(hx_result==DP_OK){dp_quality_mass(&quality,dp_hx_mg(hx_raw,0),true,now);hx_count++;}
  else if(hx_result!=DP_NOT_READY)dp_quality_mass(&quality,0,false,now);
 }
 if((uint32_t)(now-last_adc)>=100){last_adc=now;adc_valid=read_adc()==DP_OK;if(adc_valid)adc_filtered=dp_quality_target(&quality,adc_raw);adc_count++;}
 (void)dp_quality_stable(&quality,now);
 if(rx_lost){uint32_t mask=__get_PRIMASK();__disable_irq();rx_tail=rx_head;rx_lost=false;__set_PRIMASK(mask);discard=true;command_size=0;puts("ERR rx_overflow");}
 while(rx_tail!=rx_head){uint8_t c=rx_ring[rx_tail];rx_tail=(rx_tail+1)%sizeof(rx_ring);
  if(c=='\r')continue;
  if(c=='\n'){if(discard)puts("ERR length");else {command_line[command_size]=0;command(command_line);}command_size=0;discard=false;}
  else if(command_size<sizeof(command_line)-1&&!discard)command_line[command_size++]=(char)c;else discard=true;
 }
 if((uint32_t)(now-last_report)>=500){last_report=now;status();}
}
