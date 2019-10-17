#ifndef _UTIL_H_
#define _UTIL_H_

#include <cc2510fx.h>

#define BV(x) (1 << x)

#define LED_INIT P2DIR |= BV(1) | BV(2)
#define LED_BOOST_ON P2_2 = 1
#define LED_BOOST_OFF P2_2 = 0
#define LED_ON P2_1 = 1
#define LED_OFF P2_1 = 0
#define LED_TOGGLE P2_1 ^= 1

#endif