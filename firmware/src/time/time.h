#ifndef _TIME_H_
#define _TIME_H_

#include <stdint.h>

void time_init();
uint32_t time();
void sleep(uint16_t millis);

#endif