#include "display/epd.h"
#include "time/time.h"
#include "util.h"
#include "isr.h"

void main(void)
{
  time_init();
  epd_init();
  HAL_ENABLE_INTERRUPTS();

  LED_INIT;
  LED_BOOST_ON;
  while (1)
  {
    sleep(500);
    LED_TOGGLE;
  }
}