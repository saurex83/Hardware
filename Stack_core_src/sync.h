#pragma once
#include "stdbool.h"
#include "radio.h"
#include "alarm_timer.h"

#define SYNC_MODEL SYNC
struct SYNC{
  char mode;
  bool synced;
  channel_t sys_channel;
  channel_t sync_channel;
  int sync_err;
  unsigned long next_sync_send;
  unsigned long next_time_recv;
  unsigned long last_time_recv;
  char panid;
};


bool network_sync(ustime_t timeout);