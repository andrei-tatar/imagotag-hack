#include "display/epd.h"

#include "hal/clock.h"
#include "hal/hal.h"
#include "hal/isr.h"
#include "hal/led.h"
#include "hal/time.h"
#include "hal/uart.h"

#include "cobs/cobs.h"

void main(void) {
  init_clock();
  time_init();
  uart_init();
  
  HAL_ENABLE_INTERRUPTS();
  LED_INIT;

  for (uint8_t i = 0; i < 10; i++) {
    LED_TOGGLE;
    delay_ms(50);
  }

  // epd_init();

  LED_BOOST_ON;

  CobsState __xdata cobsState;
  cobs_init(&cobsState);

  uint32_t __xdata next = 0;
  while (1) {
    if (cobs_handle(&cobsState)) {
      cobs_send(cobsState.packet, cobsState.packet_size);
      LED_TOGGLE;
    }

    // if (millis() >= next) {
    //   next += 1000;
    //   LED_TOGGLE;
    // }
  }
}