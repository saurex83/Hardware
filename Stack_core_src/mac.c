#include "action_manager.h"
#include "frame.h"
#include "coder.h"
#include "radio.h"
#include "ustimer.h"
#include "debug.h"
#include "model.h"

#define RECV_TIMEOUT 2500

static void SW_Init(void);
static void IRQ_Init(void);

module_s MAC_MODULE = {ALIAS(SW_Init), ALIAS(IRQ_Init)};

struct ack{ // Формат структуры пакета ACK
  char CRC8;
} __attribute__((packed));

static void SW_Init(void){};
static void IRQ_Init(void){};


static inline struct frame* _recv_frame(channel_t ch){
  if(!RI_SetChannel(ch))
    HALT("Wrong channel");
  
  //UST_delay(400);
  nwtime_t NOW1 = AT_time();
  struct frame *frame = RI_Receive(RECV_TIMEOUT); //
  nwtime_t NOW2 = AT_time();
  return frame;
}

static uint8_t xor_calc(struct frame *frame){
  uint8_t crc = 0x34; // Начальное значение
  uint8_t *val = (uint8_t*)frame->payload;
  for (uint8_t i = 0; i < frame->len; i++)
    crc ^= val[i];
  return crc;
}

static inline void _send_ack(struct frame *frame){
  struct ack ack;
  struct frame *ack_frame;
  // Создаем подтверждение кадра
  ack.CRC8 = xor_calc(frame);
  // Создаем кадр для отправки
  ack_frame = FR_create();
  FR_add_header(ack_frame, &ack, sizeof(struct ack));
  ack_frame->meta.SEND_TIME = 0;
  RI_Send(ack_frame);
  FR_delete(ack_frame);  
}

void MAC_Receive(channel_t ch){
  struct frame *frame = _recv_frame(ch);
  if (!frame)
    return;
  
  frame->meta.TS = MODEL.TM.timeslot;
  if (MODEL.TM.timeslot > 1) // не системный таймслот требует подтверждения
    _send_ack(frame);
  
  AES_StreamCoder(false, frame->payload, frame->payload, frame->len);
  
  LOG_ON("push");
  FR_set_rx(frame);  
}

static inline bool _send_frame(struct frame *frame){
  if(!RI_SetChannel(frame->meta.CH))
    HALT("Wrong channel");  
  
  UST_delay(918);
  bool tx_success = RI_Send(frame);
  return tx_success;
}

static inline bool _recv_ack(struct frame *frame){
  return true;
}

int MAC_Send(struct frame *frame){
  bool tx_success = _send_frame(frame);
  
  bool ack_success = false;
  if (MODEL.TM.timeslot > 1) // Ждем подтверждения ack для не системных слотов
    ack_success = _recv_ack(frame);
  else
    ack_success = true;
  
  if (tx_success && ack_success){ // Удачная передача
    FR_delete(frame);
    return 1;
  }
  else{ // неудачная передача
    if (frame->meta.tx_attempts > 0)
      frame->meta.tx_attempts --;
    
    if (!frame->meta.tx_attempts){ // кончились попытки передачи
      FR_delete(frame);
      return -1;
    }
  }
  return 0;
}
