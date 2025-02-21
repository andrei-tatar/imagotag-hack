#include "cobs.h"

void cobs_init(CobsState *state) {
  state->packet_index = 0;
  state->block = 0;
  state->code = 0xFF;
}

bool cobs_handle(CobsState *state) {
  uint8_t data;
  if (!uart_read_byte(&data)) {
    return false;
  }

  if (state->packet_index == MAX_PACKET_SIZE) {
    // packet is too long,
    cobs_init(state);
  }

  if (state->block) {
    state->packet[state->packet_index++] = data;
  } else {
    state->block = data;
    if (state->block && (state->code != 0xff))
      state->packet[state->packet_index++] = 0;
    state->code = state->block;
    if (!state->code) {
      state->packet_size = state->packet_index;
      cobs_init(state);
      return true;
    }
  }

  state->block--;
  return false;
}

void cobs_send(const uint8_t *data, uint8_t length) {
  uint8_t index = 0;
  uint8_t code = 1;
  while (index < length) {
    uint8_t value = data[index];
    if (code == 1) {
      for (uint8_t i = index; i < length && data[i]; i++) {
        code++;
      }
      uart_send_byte(code);
    } else {
      code--;
    }
    if (value) {
      uart_send_byte(value);
    }
    index++;
  }
  if (code == 1) {
    uart_send_byte(code);
  }
  uart_send_byte(0);
}