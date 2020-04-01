#pragma once
#include "alarm_timer.h"

/**
@brief Представление модели модуля
*/

typedef char timeslot_t;

void Neocore_start(void);
void TM_SetAlarm(timeslot_t slot, char alarm);
void TM_ClrAlarm(timeslot_t slot, char alarm);
void TM_IRQ(nwtime_t time);

