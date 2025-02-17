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

  const char *loremIpsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
                           "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
                           "Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. "
                           "Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. "
                           "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.\r\n";

  uint32_t next = 0;
  while (1) {
    uint8_t rx;
    if (uart_read(&rx)) {
      uart_send_byte(rx);
      LED_TOGGLE;
    }

    if (millis() >= next) {
      next += 1000;
      LED_TOGGLE;
      uart_send_str(loremIpsum);
    }
  }
}