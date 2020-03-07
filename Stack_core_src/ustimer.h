#pragma once
#include "stdbool.h"


#define TICK_NS  31.25 // Количество нс в одном такте TIM2
#define TICK_US  0.03125 // Количество мкс в одном такте TIM2
#define TICK_PER_US 32 // Количество тактов в одной микросекунде
#define TICKS_TO_US(tick) ((tick)/TICK_PER_US)
#define US_TO_TICKS(us)   ((us)*TICK_PER_US)

typedef unsigned long ustime_t;
typedef unsigned long stamp_t;

stamp_t UST_now(void);
ustime_t UST_interval(stamp_t beg, stamp_t end);
void UST_delay(ustime_t time);
bool UST_time_over(stamp_t beg, ustime_t wait);
