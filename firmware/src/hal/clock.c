#include "hal.h"

void init_clock(void) {
  CLKCON = 0x80; // 32 KHz clock osc, 26MHz crystal osc.

  // wait for selection to be active
  while (!CLOCKSOURCE_XOSC_STABLE()) {
  }
  NOP();

  // power down the unused oscillator
  SLEEP |= 0x04;
 
}