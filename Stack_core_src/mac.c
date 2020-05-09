#include "action_manager.h"
#include "frame.h"
#include "coder.h"
#include "radio.h"
#include "ustimer.h"
#include "debug.h"
#include "model.h"

#define RECV_TIMEOUT 2500
#define ACK_RECV_TIMEOUT 1500

//ДЛЯ ОЛАДКИ 
nwtime_t FRAME_END ;
nwtime_t ACK_START ;

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
 // nwtime_t NOW1 = AT_time();
  struct frame *frame = RI_Receive(RECV_TIMEOUT); //
 // FRAME_END = AT_time(); // ДЛЯ ОТЛАДКИ
 // nwtime_t NOW2 = AT_time();
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
//nwtime_t T1 = AT_time(); // ДЛЯ ОТЛАДКИ
  struct ack ack;
  struct frame *ack_frame;
  // Создаем подтверждение кадра
  ack.CRC8 = xor_calc(frame);
  // Создаем кадр для отправки

  ack_frame = FR_create();

  ASSERT(ack_frame);
  FR_add_header(ack_frame, &ack, sizeof(struct ack));
//  nwtime_t T2 = AT_time(); // ДЛЯ ОТЛАДКИ
//UST_delay(500);
//ACK_START = AT_time(); // ДЛЯ ОТЛАДКИ
  
  // ВНИМАНИЕ!
  // ВЫЧИЛСЕНИЕ XOR+СОЗДАНИЕ ФРЕЙМА И ДОБАВЛЕНИЕ ACK ЗАНИМАЕТ 0.3мс!
  // Так что перед приемом нужно подождать
  RI_Send(ack_frame);
  LOG_ON("ACK=%d, frame_len=%d, TS_ACK=%d, TS_FRAME=%d", ack.CRC8, frame->len, 
         ack_frame->meta.TIMESTAMP, frame->meta.TIMESTAMP);
 // LOG_ON("ACK_S=%d, ACK_P=%d, FR_END=%d, DEL=%d, DEL_CALC=%d",
   //      ACK_START, ack_frame->meta.TIMESTAMP,FRAME_END, ACK_START-FRAME_END,
     //    T2 - T1 );
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
  
  LOG_ON("RX frame. TS=%d, CH=%d", frame->meta.TS, frame->meta.CH);
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
  // Принимаем пакет ACK на той же частоте что и передавали пакет
  // Передача ACK требует больше времени чем процедура приема.
  // Задержка была найдена эксперементально.
  // Я использую 900, но возможно 800 более оптимально будет с учетом
  // разбороса параметров узлов
  // OK[500-900] NOTGOOD[1000] BAD[1200] 
  UST_delay(900); 
  struct frame *fr_ack = RI_Receive(ACK_RECV_TIMEOUT);  
  if (!fr_ack){
    LOG_ON("Frame ack not received");
    return false;
  };
  
  // Размер ответа не верен
  if (fr_ack->len != sizeof(struct ack)){
    LOG_ON("ACK size %d incorrect. Expected %d",
           fr_ack->len, sizeof(struct ack));
    FR_delete(fr_ack);
    return false;
  }
  
  // Извлекаем структур ответа
  struct ack *ack = (struct ack*)fr_ack->payload;
  
  // Расчитываем CRC8 отправленного пакета
  uint8_t CRC8 = xor_calc(frame);
  
  // Сравним значения
  if (ack->CRC8 == CRC8){
    LOG_ON("Frame acked");
    FR_delete(fr_ack);
    return true;
  }
  else{
    LOG_ON("Frame not acked. fr_ack=%d, send_ack=%d",
           ack->CRC8, CRC8);
    FR_delete(fr_ack);
    return false;
  }
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
      LOG_ON("TX attempts exied. Frame delete.");
      return -1;
    }
  }
  return 0;
}
