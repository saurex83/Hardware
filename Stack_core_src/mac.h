#pragma once
#include "radio.h"

void MAC_Receive(channel_t ch);
int MAC_Send(struct frame *frame);