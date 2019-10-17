#include "epd.h"
#include "../util.h"

#define EPD_PWR 0   //P0_0
#define EPD_CS 1    //P0_1
#define EPD_DC 2    //P1_2
#define EPD_BUSY 3  //P1_3
#define EPD_RESET 0 //P2_0

void epd_init()
{
    PERCFG &= ~0x01;                                   // USART0 alternative 1 location
    P0SEL |= BV(3) | BV(5);                            // MISO/MOSI/CLK peripheral functions
    P0DIR |= BV(3) | BV(5) | BV(EPD_PWR) | BV(EPD_CS); // MOSI/CLK output
    U0CSR = 0;                                         // SPI mode/master/clear flags
    U0GCR = (1 << 5) | 17;                             // SCK-low idle, DATA-1st clock edge, MSB first + baud E
    U0BAUD = 0;                                        // baud M
    U0CSR |= BV(6);                                    // enable SPI

    P1DIR |= BV(EPD_DC);
    P1DIR &= ~BV(EPD_BUSY);
    P2DIR |= BV(EPD_RESET);
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