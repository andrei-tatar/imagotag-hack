#ifndef _UART_H_
#define _UART_H_

#include "hal.h"
#include <stdint.h>

void uart_init(void);
void uart_send(const uint8_t *data, uint8_t len);
void uart_send_str(const char *str);

#endif