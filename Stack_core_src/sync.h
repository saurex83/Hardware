#pragma once
#include "stdbool.h"
#include "radio.h"
#include "alarm_timer.h"

void SY_restart();
bool network_sync(ustime_t timeout);