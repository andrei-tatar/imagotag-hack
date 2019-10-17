#include "display/epd.h"
#include "util.h"

void main(void)
{
  epd_init();
  LED_INIT;

  LED_BOOST_ON;
  while (1)
  {
    LED_TOGGLE;
  }
}