#include "time.h"
#include "../util.h"

static volatile uint32_t currentTime = 0;

INTERRUPT(time_isr, WDT_VECTOR)
{
    currentTime += 2;
    IRCON2 &= ~BV(4); // clear flag
}

void time_init()
{
    WDCTL = BV(3) | BV(2) | 3; // enable WD, timer mode - 2ms
    IEN2 |= BV(5);             // enable WD interrupt
}

volatile uint32_t time()
{
    uint32_t value;
    HAL_CRITICAL_STATEMENT(value = currentTime);
    return value;
}

void delay(uint16_t millis)
{
    uint32_t start = time();
    while (time() - start < millis)
    {
    }
}