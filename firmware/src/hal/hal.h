#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdbool.h>

#ifndef BUILD

#undef SDCC
#undef __SDCC
#define INTERRUPT(name, vector) void name(void)
#define __xdata

#endif

#include <cc2510fx.h>

#define BV(x) (1 << (x))
#define st(x) \
  do {        \
    x         \
  } while (__LINE__ == -1)

#define HAL_ENABLE_INTERRUPTS() st(EA = 1;)
#define HAL_DISABLE_INTERRUPTS() st(EA = 0;)
#define HAL_INTERRUPTS_ARE_ENABLED() (EA)

typedef unsigned char halIntState_t;
#define HAL_ENTER_CRITICAL_SECTION(x) st(x = EA; HAL_DISABLE_INTERRUPTS();)
#define HAL_EXIT_CRITICAL_SECTION(x) st(EA = x;)
#define HAL_CRITICAL_STATEMENT(x) st(halIntState_t _s; HAL_ENTER_CRITICAL_SECTION(_s); x; HAL_EXIT_CRITICAL_SECTION(_s);)

#define CLOCKSOURCE_XOSC_STABLE() (SLEEP & 0x40)

#endif