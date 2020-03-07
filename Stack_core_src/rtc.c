#include "action_manager.h"
#include "model.h"
 
static void SW_Init(void); 
static void Cold_Start(void);
static void Hot_Start(void);

module_s RTC_MODULE = {ALIAS(SW_Init), ALIAS(Cold_Start), 
  ALIAS(Hot_Start)};
 
static void SW_Init(void){}; 
static void Cold_Start(void){};


static void Hot_Start(void){
  if (MODEL.TM.timeslot != 0)
    return;    
  MODEL.RTC.uptime++;
  MODEL.RTC.rtc++;
};