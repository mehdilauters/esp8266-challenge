#include "test.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_config.h"
#include "osapi.h"
#include "user_interface.h"


#ifdef USE_SSL
# define CONNECT(conn) espconn_secure_connect( conn )
# define DISCONNECT(conn) espconn_secure_disconnect( conn )
# define SEND(conn, buffer, len) espconn_secure_sent(conn, buffer, len)

unsigned char *default_certificate;
unsigned int default_certificate_len = 0;
unsigned char *default_private_key;
unsigned int default_private_key_len = 0;

#else
# define CONNECT(conn) espconn_connect( conn )
# define DISCONNECT(conn) espconn_disconnect( conn )
# define SEND(conn, buffer, len) espconn_sent(conn, buffer, len)
#endif

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1

os_event_t    user_procTaskQueue[user_procTaskQueueLen];


static volatile os_timer_t second_timer;


ip_addr_t m_server_ip;
struct espconn m_conn;
esp_tcp m_tcp;

int m_port = 0;
char m_buffer[MAX_BUFFER_SIZE];
int m_size = 0;
uint64_t m_total_size = 0;

volatile uint64_t m_seconds = 0;

void second_cb(void *arg) {
  //TODO LOCK
  m_seconds ++;
}

uint64_t get_seconds() {
  return m_seconds;
}

void ICACHE_FLASH_ATTR process_uart() {
  
  uint8 uart_buf[128]={0};
  uint16 len = 0;
  len = rx_buff_deq(uart_buf, 128 );
  if(len !=0)
  {
    int i;
    for(i=0; i < len; i++) {
      os_printf("%c",uart_buf[i]);
    }
  }
}


static void ICACHE_FLASH_ATTR
user_procTask(os_event_t *events)
{
  process_uart();
  os_delay_us(1000);
  system_os_post(user_procTaskPrio, 0, 0 );
}

void test_send_data(void *arg, char *_data, int _len)
{
  m_total_size += _len;
  struct espconn *conn = arg;
  SEND( conn, _data, _len );
}


void data_received( void *arg, char *pdata, unsigned short len )
{
  os_printf( "%s: %s\n", __FUNCTION__, pdata );
  if(len == 1)
  {
    if(pdata[0] == '-')
    {
      os_printf("Data transmission fails");
    }
    else if(pdata[0] == '+')
    {
      test_send_data(arg, m_buffer, m_size);
    }
    if(get_seconds() > 0)
    {
      os_printf("%f b/s\n",(float)m_total_size / (float)get_seconds());
    }
  }
  else
  {
    os_printf("protocol error");
  }
}


static void ICACHE_FLASH_ATTR tcp_reconnect(void *arg, sint8 errType)
{
  os_printf("tcp connect failed\n");
  struct espconn *pespconn = (struct espconn *) arg;
  espconn_delete(pespconn);
//   sync_done(false);
}

void tcp_disconnected( void *arg )
{
  os_printf("Tcp disconnected\n");
}

void data_sent(void *arg)
{
  os_printf("sent\n");
  struct espconn *conn = arg;
}

void test_send_test_data(void *arg)
{
  m_size = 0;
  os_memset(m_buffer, 0, MAX_BUFFER_SIZE);
  
  //fill buffer
  while(m_size < MAX_BUFFER_SIZE - strlen(TEST) -1)
  {    
    os_memcpy( m_buffer+m_size, TEST, strlen(TEST) );
  }
  
  test_send_data(arg, m_buffer, m_size);
}

void tcp_connected( void *arg )
{
  struct espconn *conn = arg;
  espconn_regist_recvcb( conn, data_received );
  espconn_regist_sentcb( conn, data_sent);
  
  //Start os task
  system_os_task(user_procTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
  system_os_post(user_procTaskPrio, 0, 0 );
  
#ifdef TEST
  test_send_test_data(arg);
#endif
}

void tcp_connect_start( void *arg )
{
  uint8_t count;
  struct espconn *conn = arg;
  
  for(count = 0; count < MAX_TRIES; count++) {
    if(CONNECT(conn)) {
      break;
    }
  }
}

void dns_done( const char *name, ip_addr_t *ipaddr, void *arg )
{
  if ( ipaddr == NULL) 
  {
    os_printf("DNS lookup failed\n");
  }
  else
  {
    os_printf("found server %d.%d.%d.%d\n",
              *((uint8 *)&ipaddr->addr), *((uint8 *)&ipaddr->addr + 1), *((uint8 *)&ipaddr->addr + 2), *((uint8 *)&ipaddr->addr + 3));
    
    struct espconn *conn = arg;
    
    conn->type = ESPCONN_TCP;
    conn->state = ESPCONN_NONE;
    conn->proto.tcp=&m_tcp;
    conn->proto.tcp->local_port = espconn_port();
    conn->proto.tcp->remote_port = m_port;
    os_memcpy( conn->proto.tcp->remote_ip, &ipaddr->addr, 4 );
    
    espconn_regist_connectcb( conn, tcp_connected );
    espconn_regist_disconcb( conn, tcp_disconnected );
    espconn_regist_reconcb(conn, tcp_reconnect);
    
    
    tcp_connect_start(arg);
  }
}


void test_start(const char *_server, int _port)
{
  os_timer_disarm(&second_timer);
  os_timer_setfn(&second_timer, (os_timer_func_t *) second_cb, NULL);
  os_timer_arm(&second_timer, 1000, 1);
  
  
  m_port = _port;
  err_t res = espconn_gethostbyname( &m_conn, _server, &m_server_ip, dns_done );
  if(res != ESPCONN_OK && res != ESPCONN_INPROGRESS) {
    os_printf("DNS error %d\n",res);
  }
}