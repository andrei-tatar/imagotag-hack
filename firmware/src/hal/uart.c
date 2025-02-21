#include "uart.h"
#include <stdbool.h>
#include <string.h>

#define UART_TX_BUFFER_SIZE 128
#define UART_RX_BUFFER_SIZE 128

uint8_t __xdata tx_buffer[UART_TX_BUFFER_SIZE];
volatile uint8_t tx_buffer_head = 0;
volatile uint8_t tx_buffer_tail = 0;
volatile bool tx_in_progress = false;
volatile bool tx_buffer_full = false;

uint8_t __xdata rx_buffer[UART_TX_BUFFER_SIZE];
volatile uint8_t rx_buffer_head = 0;
volatile uint8_t rx_buffer_tail = 0;
volatile bool rx_buffer_full = false;

INTERRUPT(uart_tx_isr, UTX1_VECTOR) {
  UTX1IF = 0;

  if (tx_buffer_head == tx_buffer_tail && !tx_buffer_full) {
    // buffer is empty, nothing to send
    tx_in_progress = false;
  } else {
    U1DBUF = tx_buffer[tx_buffer_tail];
    tx_buffer_tail = (tx_buffer_tail + 1) % UART_TX_BUFFER_SIZE;
    tx_buffer_full = false;
  }
}

INTERRUPT(uart_rx_isr, URX1_VECTOR) {
  URX1IF = 0;

  if (rx_buffer_full) {
    // buffer is full, discard the byte, flag an error
    return;
  }

  rx_buffer[rx_buffer_head] = U1DBUF;
  rx_buffer_head = (rx_buffer_head + 1) % UART_RX_BUFFER_SIZE;
  rx_buffer_full = rx_buffer_head == rx_buffer_tail;
}

void uart_init(void) {
  // USART1 use ALT1
  PERCFG &= ~BV(1);
  // USART1 has priority when USART0 is also enabled
  P2DIR = (P2DIR & 0x3F) | BV(6);
  // configure pin P0_4 (TX) and P0_5 (RX) as special function:
  P0SEL |= BV(4) | BV(5);

  P0DIR |= BV(4);         // make tx pin output
  P0DIR &= ~BV(5);        // make rx pin input
  U1CSR |= BV(7) | BV(6); // uart mode + enable RX
  U1UCR = BV(1);          // high stop bit

  // 115200 baud, 26MHz clock
  U1BAUD = 34;
  U1GCR = 12;

  IEN2 |= BV(3); // enable TX interrupt
  URX1IE = 1;    // enable RX interrupt
}

void uart_send_byte(uint8_t data) {
  while (tx_buffer_full) {
    // buffer is full, wait for a byte to be sent
  }

  tx_buffer[tx_buffer_head] = data;

  HAL_CRITICAL_STATEMENT({
    tx_buffer_head = (tx_buffer_head + 1) % UART_TX_BUFFER_SIZE;
    tx_buffer_full = tx_buffer_head == tx_buffer_tail;
  });

  if (!tx_in_progress) {
    // kick off a transmission
    tx_in_progress = true;
    UTX1IF = 1;
  }
}

void uart_send(const uint8_t *data, size_t len) {
  while (len--) {
    uart_send_byte(*data++);
  }
}

void uart_send_str(const char *str) {
  uart_send(str, strlen(str));
}

uint8_t uart_available() {
  uint8_t head, tail;
  HAL_CRITICAL_STATEMENT({
    head = rx_buffer_head;
    tail = rx_buffer_tail;
  });
  if (head >= tail) {
    return head - tail;
  }
  return UART_RX_BUFFER_SIZE - tail + head;
}

bool uart_read_byte(uint8_t *data) {
  bool isBufferEmpty;
  HAL_CRITICAL_STATEMENT({
    isBufferEmpty = rx_buffer_head == rx_buffer_tail && !rx_buffer_full;
  });
  if (isBufferEmpty) {
    return false;
  }

  *data = rx_buffer[rx_buffer_tail];
  rx_buffer_tail = (rx_buffer_tail + 1) % UART_RX_BUFFER_SIZE;
  rx_buffer_full = false;
  return true;
}
