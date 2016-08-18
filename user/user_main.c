#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "user_config.h"
#include "uart.h"
#include "test.h"

#include "user_interface.h"



void user_rf_pre_init(void) {
}


void wifi_callback( System_Event_t *evt )
{  
  switch ( evt->event )
  {
    case EVENT_STAMODE_CONNECTED:
    {
      break;
    }
    
    case EVENT_STAMODE_DISCONNECTED:
    {
      break;
    }
    
    case EVENT_STAMODE_GOT_IP:
    {
      os_printf("ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
        IP2STR(&evt->event_info.got_ip.ip),
        IP2STR(&evt->event_info.got_ip.mask),
        IP2STR(&evt->event_info.got_ip.gw));
      os_printf("\n");
      test_start(SERVER, PORT);
      
      break;
    }
    
    default:
    {
      break;
    }
  }
}

bool connect(const char* _essid, const char * _key)
{
  wifi_set_event_handler_cb( wifi_callback );
  
  wifi_set_opmode(STATION_MODE);
  static struct station_config config;
  config.bssid_set = 0;
  os_memcpy( &config.ssid, _essid, strlen(_essid) );
  os_memcpy( &config.password, _key, strlen(_key) );
  wifi_station_set_config_current( &config );
  wifi_station_connect();
}

//Init function 
void ICACHE_FLASH_ATTR
user_init()
{
  uart_init(BIT_RATE_115200, BIT_RATE_115200);
  os_printf("Hello\n");
  
  connect(ESSID, PWD);
}
