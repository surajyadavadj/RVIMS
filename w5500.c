#include "w5500.h"
#include "w5500_spi.h"
#include <usart_debug.h>
#include <stdio.h>

// STEP-3 : NETWORK CONFIG  

static const uint8_t MAC[6] = {0x00,0x08,0xDC,0x11,0x22,0x33};
static const uint8_t IP [4] = {192,168,1,50};
static const uint8_t SN [4] = {255,255,255,0};
static const uint8_t GW [4] = {192,168,1,1};

void w5500_net_config(void)
{
    uint8_t rd[6];
    char buf[64];

    usart_debug("STEP-3: NET CFG WRITE...\r\n");

    w5500_write_buf(0x0009, 0x00, MAC, 6); // SHAR
    w5500_write_buf(0x000F, 0x00, IP , 4); // SIPR
    w5500_write_buf(0x0005, 0x00, SN , 4); // SUBR
    w5500_write_buf(0x0001, 0x00, GW , 4); // GAR

    w5500_read_buf(0x0009, 0x00, rd, 6);
    sprintf(buf, "MAC %02X:%02X:%02X:%02X:%02X:%02X\r\n",
            rd[0],rd[1],rd[2],rd[3],rd[4],rd[5]);
    usart_debug(buf);

    w5500_read_buf(0x000F, 0x00, rd, 4);
    sprintf(buf, "IP  %d.%d.%d.%d\r\n",
            rd[0],rd[1],rd[2],rd[3]);
    usart_debug(buf);

    w5500_read_buf(0x0005, 0x00, rd, 4);
    sprintf(buf, "SN  %d.%d.%d.%d\r\n",
            rd[0],rd[1],rd[2],rd[3]);
    usart_debug(buf);

    w5500_read_buf(0x0001, 0x00, rd, 4);
    sprintf(buf, "GW  %d.%d.%d.%d\r\n",
            rd[0],rd[1],rd[2],rd[3]);
    usart_debug(buf);
}

// STEP-4 : TCP SOCKET

#define SOCK0_BLOCK  0x01

#define Sn_MR   0x0000
#define Sn_CR   0x0001
#define Sn_SR   0x0003
#define Sn_PORT 0x0004
#define CMD_OPEN    0x01
#define CMD_LISTEN  0x02
#define CMD_CLOSE   0x10
#define SOCK_INIT        0x13
#define SOCK_LISTEN      0x14
#define SOCK_ESTABLISHED 0x17
#define Sn_RX_RSR  0x0026
#define Sn_RX_RD   0x0028
#define CMD_RECV   0x40
#define RXBUF_BASE 0x6000
#define RXBUF_MASK   0x07FF   // 2KB RX buffer
#define Sn_RXBUF    0x18
#define RXBUF_BLOCK 0x02   // socket RX buffer
#define SOCK0_RX_BLOCK   0x02    // socket 0 RX buffer block
#define RX_BUF_MASK     0x07FF  // 2KB buffer wrap mask (default)
#define Sn_TX_FSR   0x0020
#define Sn_TX_WR    0x0024
#define CMD_SEND    0x20
#define SOCK0_TX_BLOCK  0x02   // socket 0 TX buffer = 0x03

#define TX_BUF_MASK     0x07FF

#define CMD_CONNECT  0x04
#define Sn_DIPR   0x000C   // Destination IP
#define Sn_DPORT  0x0010   // Destination Port

static void sock0_write8(uint16_t reg, uint8_t v)
{
    w5500_write(reg, SOCK0_BLOCK, v);
}

static uint8_t sock0_read8(uint16_t reg)
{
    return w5500_read(reg, SOCK0_BLOCK);
}

static uint16_t sock0_read16(uint16_t reg)
{
    uint16_t v;
    v  = w5500_read(reg, SOCK0_BLOCK) << 8;
    v |= w5500_read(reg + 1, SOCK0_BLOCK);
    return v;
}

static void sock0_write16(uint16_t reg, uint16_t v)
{
    w5500_write(reg,     SOCK0_BLOCK, (v >> 8) & 0xFF);
    w5500_write(reg + 1, SOCK0_BLOCK,  v & 0xFF);
}



uint8_t w5500_tcp_is_connected(void)
{
    return (sock0_read8(Sn_SR) == SOCK_ESTABLISHED);
}

void w5500_tcp_client_connect(uint8_t *ip, uint16_t port)
{
    usart_debug("TCP CLIENT MODE (SOCKET-0)\r\n");

    sock0_write8(Sn_CR, CMD_CLOSE);
    for (volatile int i = 0; i < 30000; i++);

    sock0_write8(Sn_MR, 0x01);          // TCP
    sock0_write16(Sn_PORT, 40000);      // local port

    w5500_write_buf(Sn_DIPR, SOCK0_BLOCK, ip, 4);
    sock0_write16(Sn_DPORT, port);

    sock0_write8(Sn_CR, CMD_OPEN);
    for (volatile int i = 0; i < 30000; i++);

    sock0_write8(Sn_CR, CMD_CONNECT);

    //   wait for ESTABLISHED
    while (1)
    {
        uint8_t sr = sock0_read8(Sn_SR);
        if (sr == SOCK_ESTABLISHED)
        {
            usart_debug("TCP CONNECTED TO PC\r\n");
            break;
        }
    }
}


uint16_t w5500_tcp_recv(uint8_t *buf, uint16_t maxlen)
{
    uint16_t rx_size;
    uint16_t rx_rd;
    uint16_t offset;
    uint16_t i;

    // 1. RX received size 
    rx_size  = w5500_read(Sn_RX_RSR, SOCK0_BLOCK) << 8;
    rx_size |= w5500_read(Sn_RX_RSR + 1, SOCK0_BLOCK);

    if (rx_size == 0)
        return 0;

    if (rx_size > maxlen)
        rx_size = maxlen;

    // 2. RX read pointer 
    rx_rd  = w5500_read(Sn_RX_RD, SOCK0_BLOCK) << 8;
    rx_rd |= w5500_read(Sn_RX_RD + 1, SOCK0_BLOCK);

    // 3. Read RX buffer (circular) 
    for (i = 0; i < rx_size; i++)
    {
        offset = rx_rd & RX_BUF_MASK;
        buf[i] = w5500_read(offset, SOCK0_RX_BLOCK);
        rx_rd++;
    }

    // 4. Update RX read pointer 
    w5500_write(Sn_RX_RD,     SOCK0_BLOCK, (rx_rd >> 8) & 0xFF);
    w5500_write(Sn_RX_RD + 1, SOCK0_BLOCK,  rx_rd & 0xFF);

    // 5. Notify W5500 
    w5500_write(Sn_CR, SOCK0_BLOCK, CMD_RECV);

    return rx_size;
}
// TCP SEND 
void w5500_tcp_send(const uint8_t *buf, uint16_t len)
{
    uint16_t tx_free;
    uint16_t tx_wr;
    uint16_t offset;
    uint16_t i;

    // wait for free TX space 
    do {
        tx_free  = w5500_read(Sn_TX_FSR, SOCK0_BLOCK) << 8;
        tx_free |= w5500_read(Sn_TX_FSR + 1, SOCK0_BLOCK);
    } while (tx_free < len);

    // read TX write pointer 
    tx_wr  = w5500_read(Sn_TX_WR, SOCK0_BLOCK) << 8;
    tx_wr |= w5500_read(Sn_TX_WR + 1, SOCK0_BLOCK);

    // write data to TX buffer 
    for (i = 0; i < len; i++)
    {
        offset = tx_wr & TX_BUF_MASK;
        w5500_write(offset, SOCK0_TX_BLOCK, buf[i]);
        tx_wr++;
    }

    // update TX write pointer 
    w5500_write(Sn_TX_WR,     SOCK0_BLOCK, (tx_wr >> 8) & 0xFF);
    w5500_write(Sn_TX_WR + 1, SOCK0_BLOCK,  tx_wr & 0xFF);

    // send 
    w5500_write(Sn_CR, SOCK0_BLOCK, CMD_SEND);
    while (sock0_read8(Sn_CR));   // wait till send completes
}


void w5500_tcp_server_socket0(uint16_t port)
{
    char buf[64];
    uint8_t sr;

    usart_debug("STEP-4: TCP SERVER (SOCKET-0)\r\n");

    sock0_write8(Sn_CR, CMD_CLOSE);
    for (volatile int i = 0; i < 30000; i++);

    sock0_write8(Sn_MR, 0x01);       // TCP
    sock0_write16(Sn_PORT, port);

    sock0_write8(Sn_CR, CMD_OPEN);
    for (volatile int i = 0; i < 30000; i++);

    sr = sock0_read8(Sn_SR);
    sprintf(buf, "Sn_SR after OPEN = 0x%02X\r\n", sr);
    usart_debug(buf);

    if (sr != SOCK_INIT) return;

    sock0_write8(Sn_CR, CMD_LISTEN);
    for (volatile int i = 0; i < 30000; i++);

    sr = sock0_read8(Sn_SR);
    sprintf(buf, "Sn_SR after LISTEN = 0x%02X\r\n", sr);
    usart_debug(buf);

    if (sr != SOCK_LISTEN) return;

    usart_debug("TCP SERVER LISTENING...\r\n");

    while (1)
    {
        if (sock0_read8(Sn_SR) == SOCK_ESTABLISHED)
        {
            usart_debug("TCP CONNECTED (ESTABLISHED)\r\n");
            break;
        }
    }
}
