//****************************************************************************
//  blink1.c
//  Initial test to blink LEDs on lower 4 bits of P1
//****************************************************************************
#include <cc2510fx.h>
#include "hal/spi.h"
#include "util.h"

void delay_ms(unsigned int ct);
void delay1ms(void);

void init()
{
  //P0_0 - EPD_PWR
  //P0_1 - EPD_CS
  P0DIR |= PIN(0) | PIN(1);

  spi_init();
}

void main(void)
{
  init();
  
  P2DIR |= PIN(1) | PIN(2);
  P2_2 = 1;
  while (1)
  {
    P2_1 ^= 1;
    delay_ms(50);

    spi_write(0x55);
    spi_write(0xDA);
  }
}

void delay_ms(unsigned int ct)
{
  while (ct)
  {
    delay1ms();
    ct--;
  }
}

void delay1ms(void)
{
  volatile unsigned char i, j;

  __asm__(";\n\n\tnop");

  i = 6;
  j = 20;
  do
  {
    while (--j)
      ;
  } while (--i);
}
