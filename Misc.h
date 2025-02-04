// $Id: Misc.h,v 1.1 2025/01/28 20:21:57 administrateur Exp $

#ifndef __MISC__
#define __MISC__

#include <sstream>

// Masques pour la recopie des états des leds
#define STATE_NO_LEDS         0
#define STATE_LED_RED         (1 << 0)
#define STATE_LED_YELLOW      (1 << 1)
#define STATE_LED_GREEN       (1 << 2)

#define LED_RED                       44
#define LED_YELLOW                    43
#define LED_GREEN                     6

#define USE_INCOMING_CMD              1

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

extern byte                     g__state_leds;

#endif
