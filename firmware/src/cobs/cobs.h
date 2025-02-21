#ifndef _COBS_H_
#define _COBS_H_

#include "../hal/hal.h"
#include "../hal/uart.h"

#define MAX_PACKET_SIZE 128

typedef struct {
  uint8_t packet[MAX_PACKET_SIZE];
  uint8_t packet_size;
  uint8_t packet_index;
  uint8_t block, code;
} CobsState;

void cobs_init(CobsState *state);
bool cobs_handle(CobsState *state);
void cobs_send(const uint8_t *data, uint8_t size);

#endif
