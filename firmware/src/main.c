#include "display/epd.h"

#include "hal/clock.h"
#include "hal/hal.h"
#include "hal/isr.h"
#include "hal/led.h"
#include "hal/time.h"
#include "hal/uart.h"

void main(void) {
  init_clock();
  HAL_ENABLE_INTERRUPTS();
  time_init();
  uart_init();

  LED_INIT;

  for (uint8_t i = 0; i < 10; i++) {
    LED_TOGGLE;
    delay_ms(50);
  }

  // epd_init();

  LED_BOOST_ON;

  uint32_t next = 0;
  while (1) {
    if (millis() >= next) {
      next += 1000;
      LED_TOGGLE;
      uart_send_str("hello world!");
    }
  }
}