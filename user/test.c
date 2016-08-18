#include "test.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_config.h"
#include "osapi.h"
#include "user_interface.h"

#include "mutex.h"

#define T uint8_t
#include "fifo.h"

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

#define feed_taskPrio        0
#define feed_taskQueueLen    10

os_event_t    feed_taskQueue[feed_taskQueueLen];
static void feed_task(os_event_t *events);


ip_addr_t m_server_ip;
struct espconn m_conn;
esp_tcp m_tcp;

int m_port = 0;
char m_send_buffer[MAX_BUFFER_SIZE];
char m_buffer[MAX_BUFFER_SIZE];
uint64_t m_to_send = 0;
fifo_t m_fifo;
mutex_t m_mutex;

volatile uint64_t m_seconds = 0;

void second_cb(void *arg) {
  //TODO LOCK
  m_seconds ++;
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
#ifndef TEST
      if(! fifo_isfull(&m_fifo))
      {
        fifo_push(&m_fifo, uart_buf[i]);
        os_printf("+");
      }
      else
      {
        os_printf("-");
      }
#endif
    }
  }
}



void test_send_data()
{
  if(GetMutex(&m_mutex))
  {
    if(fifo_size(&m_fifo) == 0)
    {
//       os_printf("No data to send\n");
      ReleaseMutex(&m_mutex);
      return;
    }
    os_memset(m_send_buffer,0, MAX_BUFFER_SIZE);
    int len = MAX_BUFFER_SIZE;
    if(fifo_size(&m_fifo) < len)
    {
      len = fifo_size(&m_fifo);
    }
    int i;
    for(i = 0; i < len; i++)
    {
      m_send_buffer[i] = fifo_pop(&m_fifo);
    }
    os_printf( ">>>>%s %s\n", __FUNCTION__, m_send_buffer);
    
    SEND( &m_conn, m_send_buffer, len );
  }
}

static void ICACHE_FLASH_ATTR
feed_task(os_event_t *events)
{
//   os_printf("*\n");
  process_uart();
  os_delay_us(1000);
  
  
#ifdef TEST
  const char * data = TEST;
  int len = os_strlen(data);
  GetMutex(&m_mutex);
  while(! fifo_isfull(&m_fifo) )
  {
    fifo_push(&m_fifo, data[m_to_send%len]);
    m_to_send ++;
  }
  ReleaseMutex(&m_mutex);
#else
  
#endif
  test_send_data();
  
  system_os_post(feed_taskPrio, 0, 0 );
}


void data_received( void *arg, char *pdata, unsigned short len )
{
  os_printf( "%s: %s\n", __FUNCTION__, pdata );
}


static void ICACHE_FLASH_ATTR tcp_reconnect(void *arg, sint8 errType)
{
  os_printf( "%s\n", __FUNCTION__);
  struct espconn *pespconn = (struct espconn *) arg;
  espconn_delete(pespconn);
//   sync_done(false);
}

void tcp_disconnected( void *arg )
{
  os_printf( "%s\n", __FUNCTION__);
}

void data_sent(void *arg)
{
  ReleaseMutex(&m_mutex);
  os_printf( "%s\n", __FUNCTION__);
  struct espconn *conn = arg;
}


void tcp_connected( void *arg )
{
  os_printf( "%s\n", __FUNCTION__);
  struct espconn *conn = arg;
  espconn_regist_recvcb( conn, data_received );
  espconn_regist_sentcb( conn, data_sent);
  
  //Start os task
  system_os_task(feed_task, feed_taskPrio,feed_taskQueue, feed_taskQueueLen);
  system_os_post(feed_taskPrio, 0, 0 );

}

void tcp_connect_start( void *arg )
{
  os_printf( "%s\n", __FUNCTION__);
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
  CreateMutux(&m_mutex);
  fifo_init(&m_fifo, m_buffer, MAX_BUFFER_SIZE+1);
  
  
  m_port = _port;
  err_t res = espconn_gethostbyname( &m_conn, _server, &m_server_ip, dns_done );
  if(res != ESPCONN_OK && res != ESPCONN_INPROGRESS) {
    os_printf("DNS error %d\n",res);
  }
}