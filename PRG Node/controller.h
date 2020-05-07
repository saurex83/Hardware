#pragma once
#include "udp.h"
#include "tcp.h"
#include "frame.h"
#include "debug.h"
#include "model.h"

bool NeocoreReady();
void HP_Init(void);
void setUserTimeAllocation(void (*fn)(void));