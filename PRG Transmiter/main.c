#include "action_manager.h"
#include "time_manager.h"
#include "debug.h"
#include "model.h"
#include "uart_comm.h"
#include "cmd_parser.h"
#include "stdlib.h"
#include "coder.h"
#include "stdio.h"
static void pre_init(void){
  AM_HW_Init();
  AM_SW_Init();
  LOG_ON("Hardware inited");
 // MODEL.SYNC.mode = 2;
 // MODEL.TM.MODE = 0;
}

void main(void){
  pre_init();
 // MODEL.TM.MODE = 0;
 // Neocore_start();
  com_uart_init();
  parse_uart_stream();
  
  unsigned long  last_rtc = MODEL.RTC.rtc;
  while (1){
    while (MODEL.RTC.rtc == last_rtc);
    LOG_ON("rtc = %d", MODEL.RTC.rtc);
    last_rtc = MODEL.RTC.rtc;
  }
};
