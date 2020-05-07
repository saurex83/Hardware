#pragma once
#include "model.h"
#include "debug.h"

void AUTH_ETH_Init();
void AUTH_ETH_Receive(struct frame *frame);
void AUTH_ETH_TimeAlloc();
struct frame* get_AUTH_NODE_REQ();
void set_AUTH_NODE_RESP(struct frame* frame);