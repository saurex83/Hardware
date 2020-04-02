#pragma once
#include "action_manager.h"
#include "alarm_timer.h"
#include "frame.h"
#include "rtc.h"
#include "time_manager.h"
#include "coder.h"
#include "radio.h"
#include "sync.h"

#define RED_ZONE 0x78

typedef struct {
    struct {int a;} AM;
    struct {int a;} AT;
    
    struct {
      unsigned long uptime;
      unsigned long rtc;
    } RTC;
    
    struct {
      char MODE;
      timeslot_t timeslot;
      char alarm;
      nwtime_t time;
    } TM;
    
    struct {int a;} FR;
    
    struct {
      char  STREAM_KEY[16];
      char  STREAM_IV[16];
      char  CCM_KEY[16];
      char  CCM_IV[16];
      ustime_t elapsed_time;
    } AES;
    
    struct {
      unsigned long CRCError;
      unsigned long CCAReject;
      power_t power_tx;
      channel_t channel;
      float UptimeTX;       // В секундах
      float UptimeRX;       // В секундах
   
   #ifdef RADIO_DEBUG
      struct {
        nwtime_t isrxon;
        nwtime_t rssistat;
        nwtime_t istxoncca;
        nwtime_t sfd; 
        nwtime_t txdone;
        char ccasampled;
        ustime_t fulltime;
      } DEBUG_TX;
      struct {
        nwtime_t isrxon;
        nwtime_t sfd; 
        nwtime_t rxdone;
        ustime_t fulltime;
        char received;
      } DEBUG_RX;  
    #endif      
        } RADIO;
        struct SYNC_MODEL SYNC;
        unsigned int node_adr;
} MODEL_s;


extern MODEL_s MODEL;