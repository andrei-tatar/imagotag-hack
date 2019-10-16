#include <cc2510fx.h>
#include "../util.h"
#include "spi.h"

void spi_init()
{
    PERCFG &= ~0x01;          // USART0 alternative 1 location
    P0SEL |= PIN(3) | PIN(5); // MISO/MOSI/CLK peripheral functions
    P0DIR |= PIN(3) | PIN(5); // MOSI/CLK output
    U0CSR = 0;                // SPI mode/master/clear flags
    U0GCR = (1 << 5) | 17;    // SCK-low idle, DATA-1st clock edge, MSB first + baud E
    U0BAUD = 0;               // baud M
    U0CSR |= PIN(6);          // enable SPI
}

uint8_t spi_write(uint8_t data)
{
    U0CSR &= ~0x02;
    U0DBUF = data;
    while (U1CSR & 0x01)
    {
    }
    return U0DBUF;
}