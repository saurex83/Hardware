#include "action_manager.h"
#include "time_manager.h"
#include "debug.h"
#include "model.h"
#include "sync.h"
#include "action_manager.h"
#include "frame.h"
#include "model.h"
#include "radio.h"
#include "llc.h"
#include "ethernet.h"

static struct frame* get_frame(void){
  struct frame *frame = FR_create(); 
  char data[10] = {1,2,3,4,5,6,7,8,9,0};
  FR_add_header(frame, data, sizeof(data));
  frame->meta.tx_attempts = 3;
  frame->meta.TS = 2;
  frame->meta.CH = CH20;
  return frame;
}

/** brief Функция вызывается после приема пакетов
* Здесь происходит обработка протоколов верхнего уровня
*/
static void callback(void){
  static bool first=true;
  if (first){
    LLC_open_slot(1, MODEL.SYNC.sys_channel);
    first = false;
  }
  ethernet_process();
//  LLC_add_tx_frame(frame);
 LOG_OFF("callback exit");
}

//TODO alarm manager вызывает из прерывания TM_IRQ
//в TM_IRQ засоряется стек прерывания
static void pre_init(void){
  AM_HW_Init();
  AM_SW_Init();
  LOG_ON("Hardware inited");
  MODEL.SYNC.mode = 1;
  MODEL.TM.MODE = 1;
  AM_set_callback(callback);
}

// TODO добавить в buffer.c размер RX и TX очереди
void main(void){
  pre_init();
  LOG_ON("Node started");
  
  while (1){
    MODEL.SYNC.mode = 1;
    MODEL.TM.MODE = 1;
    while (!network_sync(1000000U));
    LOG_ON("Synced");
    MODEL.node_TS = 5;
    MODEL.node_CH =14;
    Neocore_start();
    AM_SW_Init();
    LOG_ON("START RESYNC");
  }
};
