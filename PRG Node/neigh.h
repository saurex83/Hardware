#pragma once
#include "frame.h"

void NP_Receive(struct frame *frame);
bool comm_node_info(unsigned int *addr, char *ts, char *ch);
void NP_TimeAlloc();
void NP_Init();