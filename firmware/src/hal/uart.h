#ifndef _UART_H_
#define _UART_H_

#include "hal.h"
#include <stdint.h>
#include <string.h>

void uart_init(void);

void uart_send_byte(uint8_t data);
void uart_send(const uint8_t *data, size_t len);
void uart_send_str(const char *str);

uint8_t uart_available();
bool uart_read_byte(uint8_t *data);

#endif