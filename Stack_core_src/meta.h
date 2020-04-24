#pragma once
#include "alarm_timer.h"
#include "time_manager.h"

enum TX_METHODS {BROADCAST = 0, UNICAST = 1};

struct meta{
   nwtime_t TIMESTAMP;
   signed char RSSI_SIG; // Имя RSSI определено дефайном в ioCC2530.h
/* link quality. Calculate LQI = (CORR - a)b. Where a and b are found 
   empirically based on PER measurements as a function of the correlation 
   value. PER - packet error rate */
   char LIQ;  
   timeslot_t TS;
   char CH;
   char PID;
   unsigned int NDST;
   unsigned int NSRC;
   unsigned char NSRC_TS;
   unsigned char NSRC_CH;
   char ETX;
   unsigned int FDST;
   unsigned int FSRC;
   char IPP;
   char tx_attempts; 
} __attribute__((packed));