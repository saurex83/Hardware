#pragma once
#include "stdbool.h"

/**
@brief Представление модели модуля
*/
#define AT_MODEL AT
struct AT{
  int a;
};

#define NWTIME_TICK_US       31 //30.517578125    
#define NWTIME_TO_US(nwtick) ((nwtick)*NWTIME_TICK_US)

typedef unsigned int nwtime_t;

void AT_set_time(nwtime_t time);
void AT_set_alarm(nwtime_t time);
nwtime_t AT_time(void);
void AT_wait(nwtime_t time);
void AT_enable(bool state);
nwtime_t AT_interval(nwtime_t beg, nwtime_t end);
