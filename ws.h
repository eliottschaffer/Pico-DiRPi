#ifndef WS_H
#define WS_H

#include <stdint.h>
#include <stdbool.h>

#define WEBSOCKET_SUCCESS                          ( 0 )
#define WEBSOCKET_FAIL                             ( -1 )


#include "lwip/tcp.h"

/**
 *
 *    0                   1                   2                   3
 *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   +-+-+-+-+-------+-+-------------+-------------------------------+
 *   |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 *   |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 *   |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 *   | |1|2|3|       |K|             |                               |
 *   +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 *   |     Extended payload length continued, if payload len == 127  |
 *   + - - - - - - - - - - - - - - - +-------------------------------+
 *   |                               |Masking-key, if MASK set to 1  |
 *   +-------------------------------+-------------------------------+
 *   | Masking-key (continued)       |frame(1byte)|  Payload Data    |
 *   +-------------------------------- - - - - - - - - - - - - - - - +
 *   :                     Payload Data continued ...                :
 *   + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 *   |                     Payload Data continued ...                |
 *   +---------------------------------------------------------------+
 */

typedef struct {
    union {
        struct {
            uint8_t PAYLOADLEN:7;
            uint8_t MASK:1;
            uint8_t OPCODE:4;
            uint8_t RSV:3;
            uint8_t FIN:1;
       } bits;
        struct {
            uint8_t byte1;
            uint8_t byte0;
        } bytes;
    } meta;
    uint8_t  start;
    uint64_t length;
    union {
        uint32_t maskKey;
        uint8_t  maskBytes[4];
    } mask;
} WebsocketPacketHeader_t;

enum WebSocketOpCode {
    WEBSOCKET_OPCODE_CONTINUE = 0x0,
    WEBSOCKET_OPCODE_TEXT = 0x1,
    WEBSOCKET_OPCODE_BIN = 0x2,
    WEBSOCKET_OPCODE_CLOSE = 0x8,
    WEBSOCKET_OPCODE_PING = 0x9,
    WEBSOCKET_OPCODE_PONG = 0xA
};


#define BUF_SIZE 4096

typedef struct TCP_CLIENT_T_ {
    struct tcp_pcb *tcp_pcb;
    ip_addr_t remote_addr;
    uint8_t buffer[BUF_SIZE];
    uint8_t rx_buffer[BUF_SIZE];
    int buffer_len;
    int rx_buffer_len;
    int sent_len;
    int connected;
} TCP_CLIENT_T;

uint64_t BuildPacket(char* buffer, uint64_t bufferLen, enum WebSocketOpCode opcode, char* payload, uint64_t payloadLen, int mask, uint8_t frame_code);
int ParsePacket(struct tcp_pcb *pcb, WebsocketPacketHeader_t *header, char* buffer, uint32_t len);


#endif