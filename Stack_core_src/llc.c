#include "action_manager.h"
#include "stddef.h"
#include "macros.h"
#include "model.h"
#include "debug.h"
#include "radio.h"
#include "mac.h"


#define MAX_SLOTS 50

#define RX_ALARM (1<<0)
#define TX_ALARM (1<<1)

static void SW_Init(void);
static void Cold_Start(void);
static void Hot_Start(void);
static void IRQ_Init(void);

module_s LLC_MODULE = {ALIAS(SW_Init), ALIAS(Cold_Start), 
  ALIAS(Hot_Start), ALIAS(IRQ_Init)};

struct action{
  channel_t rx;
  struct frame *frame_tx;
};

static struct action ACTIONSLOTS[MAX_SLOTS];

static void IRQ_Init(void){
  for_each_type(struct action, ACTIONSLOTS, slot){
    slot->rx = 0;
    slot->frame_tx = NULL;
  }
};

static void SW_Init(void){
  for_each_type(struct action, ACTIONSLOTS, slot){
    slot->rx = 0;
    slot->frame_tx = NULL;
  }
};

void LLC_restart(){
  SW_Init();
}

void LLC_open_slot(timeslot_t ts, channel_t ch){
  ASSERT(ts >= 1 && ts < MAX_SLOTS);
  ASSERT(ch >= MIN_CH && ch <= MAX_CH);
  ACTIONSLOTS[ts].rx = ch;
  LOG_ON("Slot open. TS=%d, CH=%d", ts, ch);
}

void LLC_close_slot(timeslot_t ts){
  ASSERT(ts >= 1 && ts < MAX_SLOTS);
  ACTIONSLOTS[ts].rx = 0;
}

bool LLC_add_tx_frame(struct frame *frame){
  ASSERT(frame); 
  if (FR_tx_available() == 0){
    LOG_ON("NO FREE TX FRAME");
    FR_delete(frame);
    return false;
  };  
   
  AES_StreamCoder(true, frame->payload, frame->payload, frame->len); 
  FR_set_tx(frame);
  return true;
}

static void scheduler_tx(void){
  struct frame* tx_frame = NULL; 
  tx_frame = FR_find_tx(tx_frame);
  
  if (!tx_frame)
    return;
  
  if (tx_frame->meta.TS==0)
    while(1);
  
  while(tx_frame){
    ASSERT(tx_frame->meta.TS != 0);
    // Если у нас есть что передавать, берем следующий кадр из буфера
    if (ACTIONSLOTS[tx_frame->meta.TS].frame_tx){
      tx_frame = FR_find_tx(tx_frame);
      continue;
    }
    ACTIONSLOTS[tx_frame->meta.TS].frame_tx = tx_frame;
    TM_SetAlarm(tx_frame->meta.TS, TX_ALARM); 
    tx_frame = FR_find_tx(tx_frame);
  }
}

static void scheduler_rx(void){
  ASSERT(ACTIONSLOTS[0].rx == 0);
  for (char i = 1; i < MAX_SLOTS; i++)
    if (ACTIONSLOTS[i].rx)
      TM_SetAlarm(i, RX_ALARM);
    else
      TM_ClrAlarm(i, RX_ALARM);
}

static void Cold_Start(void){
// Планировщик планирует на один суперфрейм в начале ts0
  if (MODEL.TM.timeslot != 0)
    return;
  
  scheduler_tx();
  scheduler_rx();
};

static inline void receive(){
  timeslot_t ts = MODEL.TM.timeslot;
  MAC_Receive(ACTIONSLOTS[ts].rx);
}

static void transmite(void){
  timeslot_t ts = MODEL.TM.timeslot;
  struct frame *frame = ACTIONSLOTS[ts].frame_tx;
  
  if (!frame)
    HALT("Error")
  
  // Неудачные передачи учитывает MAC_Send()
  int send_res = MAC_Send(frame);
  switch (send_res) {
    case 1: { // удачная передача
      TM_ClrAlarm(ts, TX_ALARM);
      ACTIONSLOTS[ts].frame_tx = NULL;
      LOG_ON("Send success");
      break;
    }
    case 0: { // неудачная передача. ACK не получен или CCA
      LOG_ON("CCA/ACK err")
      break;
    }
    case -1: { // Исчерпаны попытки отправки
      TM_ClrAlarm(ts, TX_ALARM);
      ACTIONSLOTS[ts].frame_tx = NULL;
      LOG_ON("Attempts exired");
      break;
    }
    default:
      HALT("Error");
  };
}

static void Hot_Start(void){
  timeslot_t ts = MODEL.TM.timeslot;
  if (ts == 0)
    return;
  char alarm = MODEL.TM.alarm;
  
  if (alarm & TX_ALARM)
    transmite();
  else if (alarm & RX_ALARM)
    receive();
};