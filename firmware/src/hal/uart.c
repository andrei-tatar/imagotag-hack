#include "uart.h"
#include <stdbool.h>
#include <string.h>

#define UART_TX_BUFFER_SIZE 64

uint8_t tx_buffer[UART_TX_BUFFER_SIZE];
volatile uint8_t tx_buffer_head = 0;
volatile uint8_t tx_buffer_tail = 0;
volatile bool tx_in_progress = false;

INTERRUPT(uart_tx_isr, UTX1_VECTOR) {
  UTX1IF = 0;

  if (tx_buffer_head != tx_buffer_tail) {
    U1DBUF = tx_buffer[tx_buffer_tail];
    tx_buffer_tail = (tx_buffer_tail + 1) % UART_TX_BUFFER_SIZE;
  } else {
    tx_in_progress = false;
  }
}

void uart_init(void) {
  // USART1 use ALT1 -> Clear flag -> Port P0_4 = TX
  PERCFG &= ~BV(1);
  // USART1 has priority when USART0 is also enabled
  P2DIR = (P2DIR & 0x3F) | BV(6);
  // configure pin P0_4 (TX) and P0_5 (RX) as special function:
  P0SEL |= BV(4) /*| (1 << 5)*/;

  P0DIR |= BV(4); // make tx pin output
  U1CSR |= BV(7); // uart mode
  U1UCR = BV(1);  // high stop bit

  // 115200 baud, 26MHz clock
  U1BAUD = 34;
  U1GCR = 12;

  IEN2 |= BV(3); // enable TX interrupt
}

void uart_send_byte(uint8_t data) {
  // send data
  U1DBUF = data;

  // wait for TX to complete
  while (!(U1CSR & 0x02))
    ;

  // clear TX complete flag
  U1CSR &= ~BV(1);
}

void uart_send(const uint8_t *data, uint8_t len) {
  if (!len) {
    return;
  }

  for (uint8_t i = 0; i < len; i++) {
    while (tx_buffer_head == ((tx_buffer_tail - 1) % UART_TX_BUFFER_SIZE)) {
      // buffer is full, wait for a byte to be sent
    }

    tx_buffer[tx_buffer_head] = data[i];
    HAL_CRITICAL_STATEMENT(tx_buffer_head = (tx_buffer_head + 1) % UART_TX_BUFFER_SIZE);
  }

  if (!tx_in_progress) {
    // kick off a transmission
    tx_in_progress = true;
    UTX1IF = 1;
  }
}

void uart_send_str(const char *str) {
  uart_send(str, strlen(str));
}
