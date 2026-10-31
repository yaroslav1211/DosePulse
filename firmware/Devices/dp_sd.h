#ifndef DP_SD_H
#define DP_SD_H
#include <stdint.h>
#include <stddef.h>
#include "dp_bus.h"
typedef struct {void *ctx;uint8_t (*exchange)(void *,uint8_t);void (*select)(void *,int);void (*delay_ms)(void *,unsigned);} dp_spi;
dp_result dp_sd_start(dp_spi *s);
dp_result dp_sd_read(dp_spi *s,uint32_t sector,uint8_t data[512]);
dp_result dp_sd_write(dp_spi *s,uint32_t sector,const uint8_t data[512]);
#endif
