#include "display/epd.h"
#include "time/time.h"
#include "util.h"
#include "isr.h"

void main(void)
{
  HAL_ENABLE_INTERRUPTS();
  time_init();
  LED_INIT;

  for (uint8_t i = 0; i < 10; i++)
  {
    LED_TOGGLE;
    delay(50);
  }

  epd_init();

  LED_BOOST_ON;
  while (1)
  {
    delay(500);
    LED_TOGGLE;
  }
}