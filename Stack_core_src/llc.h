#pragma once
#include "time_manager.h"
#include "radio.h"

void LLC_close_slot(timeslot_t ts);
void LLC_open_slot(timeslot_t ts, channel_t ch);
bool LLC_add_tx_frame(struct frame *frame);