#include "epd.h"
#include "../util.h"
#include "../time/time.h"

#define B_PWR 0   //P0_0
#define B_CS 1    //P0_1
#define B_DC 2    //P1_2 - low command, high data
#define B_BUSY 3  //P1_3 - low busy
#define B_RESET 0 //P2_0 - low reset

#define EPD_PWR P0_0
#define EPD_CS P0_1
#define EPD_DC P1_2
#define EPD_BUSY P1_3
#define EPD_RESET P2_0

#define PWR_ON EPD_PWR = 0
#define PWR_OFF EPD_PWR = 1
#define RESET_ON EPD_RESET = 0
#define RESET_OFF EPD_RESET = 1

#define HRES 104
#define VRES 212

// #define HRES 152
// #define VRES 296
#define BUFFER_SIZE (HRES / 8 * VRES)

static void inline sendCommand(uint8_t cmd);
static void inline sendData(uint8_t data);

static void inline epd_waitBusy()
{
    do
    {
        // sendCommand(0x71);
    } while (EPD_BUSY == 0);
    delay(200);
}

static void epd_clearDisplay()
{
    sendCommand(0x10);
    for (uint16_t i = 0; i < BUFFER_SIZE; i++)
        sendData(0xff);

    sendCommand(0x13);
    for (uint16_t i = 0; i < BUFFER_SIZE; i++)
        sendData(0xff);
}

static void epd_refresh()
{
    sendCommand(0x12);
    delay(100);
    epd_waitBusy();
}

static void epd_sleep(void)
{
    sendCommand(0x02);
    epd_waitBusy();
    sendCommand(0x07);
    sendData(0xA5);
}

void epd_init()
{
    PERCFG &= ~0x01;       // USART0 alternative 1 location
    U0CSR = 0;             // SPI mode/master/clear flags
    U0GCR = (1 << 5) | 17; // SCK-low idle, DATA-1st clock edge, MSB first + baud E
    U0BAUD = 0;            // baud M
    U0CSR |= BV(6);        // enable SPI

    P0SEL |= BV(3) | BV(5);                        // MISO/MOSI/CLK peripheral functions
    P0DIR |= BV(3) | BV(5) | BV(B_PWR) | BV(B_CS); // MOSI/CLK, PWR/CS output
    P1DIR |= BV(B_DC);
    P1DIR &= ~BV(B_BUSY);
    P2DIR |= BV(B_RESET);

    PWR_ON;
    delay(1000);
    RESET_ON;
    delay(100);
    RESET_OFF;
    delay(100);

    sendCommand(6);
    sendData(0x17);
    sendData(0x17);
    sendData(0x17);

    sendCommand(4);
    epd_waitBusy();

    sendCommand(0);
    sendData(0x0f);
    sendData(0x0d);

    sendCommand(0x61);
    sendData(HRES);
    sendData(VRES >> 8);
    sendData(VRES);

    sendCommand(0x50);
    sendData(0x77);

    epd_clearDisplay();
    epd_refresh();
    // delay(20e3);
    // epd_image(gImage_black2, gImage_red2);
    // epd_refresh();
    epd_sleep();

    PWR_OFF;
}

static void inline sendData(uint8_t data)
{
    EPD_CS = 0;
    U0DBUF = data;
    while (U0CSR & 0x01)
    {
    }
    EPD_CS = 1;
}

static void inline sendCommand(uint8_t cmd)
{
    EPD_DC = 0;
    sendData(cmd);
    EPD_DC = 1;
}
