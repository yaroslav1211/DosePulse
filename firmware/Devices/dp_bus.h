#ifndef DP_BUS_H
#define DP_BUS_H
#include <stdint.h>
#include <stddef.h>
typedef enum {DP_OK,DP_NACK,DP_TIMEOUT,DP_ID,DP_NOT_READY,DP_RANGE} dp_result;
typedef struct {
 void *ctx;
 dp_result (*read)(void *,uint8_t,uint8_t,uint8_t *,size_t);
 dp_result (*write)(void *,uint8_t,uint8_t,uint8_t);
 int (*data)(void *);
 void (*clock)(void *,int);
 void (*delay_us)(void *,uint32_t);
} dp_bus;
#endif
