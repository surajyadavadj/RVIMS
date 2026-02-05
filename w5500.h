#ifndef _W5500_H_
#define _W5500_H_

#include <stdint.h>

// Network config (already done) 
void w5500_net_config(void);

// TCP server on Socket-0 
void w5500_tcp_server_socket0(uint16_t port);
void w5500_tcp_client_connect(uint8_t *ip, uint16_t port);

uint16_t w5500_tcp_recv(uint8_t *buf, uint16_t maxlen);
void w5500_tcp_send(const uint8_t *buf, uint16_t len);

uint8_t  w5500_tcp_is_connected(void);


#endif
