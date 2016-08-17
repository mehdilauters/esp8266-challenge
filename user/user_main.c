#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_config.h"
#include "uart.h"

#include "user_interface.h"


#define user_procTaskPrio        0
#define user_procTaskQueueLen    1

os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void user_procTask(os_event_t *events);

void user_rf_pre_init(void) {
}


//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
  uart_init(BIT_RATE_115200, BIT_RATE_115200);
  os_printf("Hello\n\r");
}
