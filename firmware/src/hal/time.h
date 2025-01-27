#ifndef _TIME_H_
#define _TIME_H_

#include "hal.h"
#include <stdint.h>

void time_init();
uint32_t millis();
void delay_ms(uint16_t millis);

#endif