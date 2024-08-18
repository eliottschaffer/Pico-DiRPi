#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ws.h"

#include "lwip/tcp.h"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

uint64_t BuildPacket(char* buffer, uint64_t bufferLen, enum WebSocketOpCode opcode, char* payload, uint64_t unmod_payloadLen, int mask, uint8_t frame_code) {
    WebsocketPacketHeader_t header;

    uint64_t payloadLen = unmod_payloadLen + 1;

    int payloadIndex = 2;
    
    // Fill in meta.bits
    header.meta.bits.FIN = 1;
    header.meta.bits.RSV = 0;
    header.meta.bits.OPCODE = opcode;
    header.meta.bits.MASK = mask;

    // Calculate length
    if(payloadLen < 126) {
        header.meta.bits.PAYLOADLEN = payloadLen;
    } else if(payloadLen < 0x10000) {
        header.meta.bits.PAYLOADLEN = 126;
    } else {
        header.meta.bits.PAYLOADLEN = 127;
    }

    buffer[0] = header.meta.bytes.byte0;
    buffer[1] = header.meta.bytes.byte1;

    // Generate mask
    header.mask.maskKey = (uint32_t)rand();
    
    // Mask payload
    if(header.meta.bits.MASK) {
        frame_code = frame_code ^ header.mask.maskBytes[0%4];
        for(uint64_t i = 0; i < payloadLen; i++) {
            payload[i] = payload[i] ^ header.mask.maskBytes[(i+1)%4];
        }
      }
    
    // Fill in payload length
    if(header.meta.bits.PAYLOADLEN == 126) {
            buffer[2] = (payloadLen >> 8) & 0xFF;
            buffer[3] = payloadLen & 0xFF;
        payloadIndex = 4;
     }

    if(header.meta.bits.PAYLOADLEN == 127) {
         buffer[2] = (payloadLen >> 56) & 0xFF;
         buffer[3] = (payloadLen >> 48) & 0xFF;
         buffer[4] = (payloadLen >> 40) & 0xFF;
         buffer[5] = (payloadLen >> 32) & 0xFF;
         buffer[6] = (payloadLen >> 24) & 0xFF;
         buffer[7] = (payloadLen >> 16) & 0xFF;
         buffer[8] = (payloadLen >> 8)  & 0xFF;
         buffer[9] = payloadLen & 0xFF;
        payloadIndex = 10;
    }

    // Insert masking key
    if(header.meta.bits.MASK) {
        buffer[payloadIndex] = header.mask.maskBytes[0];
        buffer[payloadIndex + 1] = header.mask.maskBytes[1];
        buffer[payloadIndex + 2] = header.mask.maskBytes[2];
        buffer[payloadIndex + 3] = header.mask.maskBytes[3];
        payloadIndex += 4;
    }


    // Ensure the buffer can handle the packet
    if((payloadLen + payloadIndex) > bufferLen) {
        printf("WEBSOCKET BUFFER OVERFLOW \r\n");
        return 1;
    }

    // Copy in payload
    // memcpy(buffer + payloadIndex, payload, payloadLen);

    //Input Frame Specification
    buffer[payloadIndex] = frame_code;
    for(int i = 0; i < payloadLen; i++) {
        buffer[payloadIndex + i + 1] = payload[i];
    }

    return (payloadIndex + payloadLen);

}

int ParsePacket(struct tcp_pcb *pcb, WebsocketPacketHeader_t *header, char* buffer, uint32_t len)
{
    header->meta.bytes.byte0 = (uint8_t) buffer[0];
    header->meta.bytes.byte1 = (uint8_t) buffer[1];

    if(header->meta.bits.OPCODE == WEBSOCKET_OPCODE_PING){
        char pongbuf[100];
        uint8_t pong_payload[2] = {0x00, 0x01};
        int pong_len = BuildPacket(pongbuf, 100, WEBSOCKET_OPCODE_PONG, pong_payload, sizeof(pong_payload), 1, 0x02);
                        
                // Send the pong response back over the WebSocket connection
        tcp_write(pcb, pongbuf, pong_len, TCP_WRITE_FLAG_COPY);
        tcp_output(pcb); // Trigger an immediate send


        printf("Pong sent back\n");
    }

    // Payload length
    int payloadIndex = 2;
    header->length = header->meta.bits.PAYLOADLEN;

    if(header->meta.bits.PAYLOADLEN == 126) {
        header->length = buffer[2] << 8 | buffer[3];
        payloadIndex = 4;
    }
    
    if(header->meta.bits.PAYLOADLEN == 127) {
        header->length = buffer[6] << 24 | buffer[7] << 16 | buffer[8] << 8 | buffer[9];
        payloadIndex = 10;
    }

    // Mask
    if(header->meta.bits.MASK) {
        header->mask.maskBytes[0] = buffer[payloadIndex + 0];
        header->mask.maskBytes[0] = buffer[payloadIndex + 1];
        header->mask.maskBytes[0] = buffer[payloadIndex + 2];
        header->mask.maskBytes[0] = buffer[payloadIndex + 3];
        payloadIndex = payloadIndex + 4;    
        
        // Decrypt    
        for(uint64_t i = 0; i < header->length; i++) {
                buffer[payloadIndex + i] = buffer[payloadIndex + i] ^ header->mask.maskBytes[i%4];
            }
    }

    // Payload start
    header->start = payloadIndex;

    return 0;

}