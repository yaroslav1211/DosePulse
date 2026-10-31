/* STM32 HAL GPIO I2C adapter. Wokwi C031 hardware I2C gets stuck after ACK:
 * saved VCD + ISR=0x8043/HAL_ERROR_TIMEOUT, upstream wokwi-features#744.
 * The real I2C protocol remains visible at PB8/PB9; no synthetic sensor data. */
#include "main.h"
#include "dp_i2c_gpio.h"
static void delay(unsigned us){uint16_t start=(uint16_t)TIM3->CNT;while((uint16_t)((uint16_t)TIM3->CNT-start)<us){}}
static void drive(uint16_t pin,int high){
 GPIO_InitTypeDef g={0};g.Pin=pin;g.Mode=high?GPIO_MODE_INPUT:GPIO_MODE_OUTPUT_OD;g.Pull=GPIO_PULLUP;g.Speed=GPIO_SPEED_FREQ_HIGH;
 if(!high)HAL_GPIO_WritePin(GPIOB,pin,GPIO_PIN_RESET);
 HAL_GPIO_Init(GPIOB,&g);
}
static void sda(int high){drive(GPIO_PIN_9,high);}
static void scl(int high){drive(GPIO_PIN_8,high);}
static int get_sda(void){return HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9)==GPIO_PIN_SET;}
static int raise_clock(void){scl(1);uint16_t start=(uint16_t)TIM3->CNT;while(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_8)!=GPIO_PIN_SET)if((uint16_t)((uint16_t)TIM3->CNT-start)>500)return 0;return 1;}
static void stop(void){scl(0);sda(0);delay(5);(void)raise_clock();delay(5);sda(1);delay(5);}
static dp_result start(void){sda(1);delay(5);if(!raise_clock())return DP_TIMEOUT;delay(5);if(!get_sda()){stop();return DP_TIMEOUT;}sda(0);delay(5);scl(0);return DP_OK;}
static dp_result send(uint8_t byte){
 for(unsigned i=0;i<8;i++){sda((byte&0x80)!=0);byte<<=1;delay(5);if(!raise_clock())return DP_TIMEOUT;delay(5);scl(0);}
 sda(1);delay(5);if(!raise_clock())return DP_TIMEOUT;delay(5);int ack=!get_sda();scl(0);return ack?DP_OK:DP_NACK;
}
static dp_result receive(uint8_t *byte,int more){
 uint8_t value=0;sda(1);
 for(unsigned i=0;i<8;i++){delay(5);if(!raise_clock())return DP_TIMEOUT;delay(5);value=(uint8_t)((value<<1)|get_sda());scl(0);}
 sda(!more);delay(5);if(!raise_clock())return DP_TIMEOUT;delay(5);scl(0);sda(1);*byte=value;return DP_OK;
}
void dp_i2c_gpio_start(void){
 GPIO_InitTypeDef g={0};g.Pin=GPIO_PIN_8|GPIO_PIN_9;g.Mode=GPIO_MODE_OUTPUT_OD;g.Pull=GPIO_PULLUP;g.Speed=GPIO_SPEED_FREQ_HIGH;
 HAL_GPIO_Init(GPIOB,&g);sda(1);scl(1);delay(5);
 if(!get_sda()){for(unsigned i=0;i<9;i++){scl(0);delay(5);(void)raise_clock();delay(5);}stop();}
}
dp_result dp_i2c_gpio_read(void *ctx,uint8_t address,uint8_t reg,uint8_t *bytes,size_t n){
 (void)ctx;dp_result r=start();if(r==DP_OK)r=send((uint8_t)(address<<1));if(r==DP_OK)r=send(reg);
 if(r==DP_OK)r=start();
 if(r==DP_OK)r=send((uint8_t)((address<<1)|1));
 for(size_t i=0;i<n&&r==DP_OK;i++)r=receive(bytes+i,i+1<n);
 stop();return r;
}
dp_result dp_i2c_gpio_write(void *ctx,uint8_t address,uint8_t reg,uint8_t value){
 (void)ctx;dp_result r=start();if(r==DP_OK)r=send((uint8_t)(address<<1));if(r==DP_OK)r=send(reg);if(r==DP_OK)r=send(value);stop();return r;
}
