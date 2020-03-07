#pragma once
#include "alarm_timer.h"

/**
@brief Представление модели модуля
*/

typedef char timeslot_t;

#define TM_MODEL TM
struct TM{
  char MODE;
  timeslot_t timeslot;
  char alarm;
  nwtime_t time;
};


void Neocore_start(void);
void TM_SetAlarm(timeslot_t slot, char alarm);
void TM_ClrAlarm(timeslot_t slot, char alarm);
void TM_IRQ(nwtime_t time);

