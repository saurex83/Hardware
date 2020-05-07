#pragma once
#include "frame.h"

void RP_TimeAlloc();
void RP_Init();
void RP_Receive(struct frame *frame);
void RP_Send_GW(struct frame *frame);
void RP_Send(struct frame *frame);
void RP_SendRT_GW(struct frame *frame);
void RP_SendRT_RT(struct frame *frame);
void RP_Send_COMM(struct frame *frame);