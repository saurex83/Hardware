#include "sync.h"
#include "action_manager.h"
#include "radio.h"
#include "model.h"
#include "alarm_timer.h"
#include "debug.h"
#include "frame.h"
#include "coder.h"
#include "stdlib.h"
#include "macros.h"
#include "global.h"

#define MODE_0 0 // Отклчена модуль синхронизации 
#define MODE_1 1 // Прием, ретрансляция, синхронизация
#define MODE_2 2 // Периодическое вещание
#define SYNC_TS 0 // Слот для синхронизации
#define MAGIC 0x19833891 // Проверка что пакет действительно sync
#define SYNC_TIME 163 // Точное время отправки пакета.nwtime
#define NEG_RECV_OFFSET 33 // nwtime
#define POS_RECV_OFFSET 33 // nwtime
#define SEND_PERIOD 10 // Периодичность отправки пакетов
#define RETRANSMITE_TRY 3 // Кол-во попыток отправки sync
#define PROBABILIT 40 // % вероятность одной попытки отправки 
#define UNSYNC_TIME 60 // Время в секундах рассинхронизации сети

static void SW_Init(void);
static void Cold_Start(void);
static void Hot_Start(void);
static bool send_sync(void);
static struct frame* recv_sync(void);
static char retransmite;

module_s SYNC_MODULE = {ALIAS(SW_Init), ALIAS(Cold_Start), 
  ALIAS(Hot_Start)};

struct sync{
  char sys_ch;
  char tx_power;
  char panid;
  unsigned long rtc;
  unsigned long magic;
} __attribute__((packed));

static void SW_Init(void){ 
  MODEL.SYNC.next_sync_send = 0;
  MODEL.SYNC.next_time_recv = 0;
  MODEL.SYNC.last_time_recv = 0;
  MODEL.SYNC.sys_channel = DEFAULT_SYS_CH;
  MODEL.SYNC.sync_channel = DEFAULT_SYNC_CH;
  retransmite = 0;
};

static void Cold_Start(void){
};

static inline bool validate_sync(struct sync *sync){
  bool valid = 
    (sync->magic == MAGIC) &&
    (sync->panid == MODEL.SYNC.panid);
  return valid;
}

static inline void accept_sync_data(struct sync *sync){
  MODEL.RTC.rtc = sync->rtc;
};

static inline void syncronize_timer(struct frame *frame){
  nwtime_t now = AT_time();
  // Время прошедшее с момента получения пакета
  // RI_Send корректриует время, чтобы SFD был передан в SEND_TIME
  // Поэтому нам корректировать ненужно
  nwtime_t passed = AT_interval(now, frame->meta.TIMESTAMP);  
  AT_set_time(SYNC_TIME + passed);
  MODEL.SYNC.sync_err = SYNC_TIME - frame->meta.TIMESTAMP;
  LOG_ON("Sync err %d", MODEL.SYNC.sync_err);
};

static inline void mode_1_receive_process(void){
  LOW(PIN1);
  struct frame *fr = recv_sync();
  if (!fr)
    return;
  
  struct sync *sync = (struct sync*)(fr->payload);
  if (!validate_sync(sync)){
    FR_delete(fr);
    return;
  }
  syncronize_timer(fr);
  accept_sync_data(sync);
  FR_delete(fr);
  LOG_ON("Sichronize sync RSSI = %d", fr->meta.RSSI_SIG);
  retransmite = RETRANSMITE_TRY;
  MODEL.SYNC.next_time_recv = MODEL.RTC.uptime + SEND_PERIOD;
  MODEL.SYNC.last_time_recv = MODEL.RTC.uptime;   
  HIGH(PIN1);
}

static inline bool _throw_dice(void){
  return  ((rand() % 100) <= PROBABILIT) ? true : false;
};

static inline void mode_1_retransmition_process(void){
  retransmite--;
  if (!_throw_dice())
    return;
  send_sync();
  retransmite = 0;
}

static void mode_1_process(){
 // Прием, ретрансляция, синхронизация   
  if ( MODEL.RTC.uptime >= MODEL.SYNC.next_time_recv)
    mode_1_receive_process();
  else if(retransmite)
    mode_1_retransmition_process();
  
  if (MODEL.RTC.uptime - MODEL.SYNC.last_time_recv > UNSYNC_TIME){
    MODEL.SYNC.synced = false;
    MODEL.SYNC.mode = 0;
    MODEL.TM.MODE = 0;
    LOG_ON("unsynced");
  }
}

static void mode_2_process(){
  // Периодическое вещание
  if ( MODEL.RTC.uptime < MODEL.SYNC.next_sync_send)
    return;
  MODEL.SYNC.next_sync_send = MODEL.RTC.uptime + SEND_PERIOD;
  LOW(PIN1);
  send_sync();
  HIGH(PIN1);
}

static void Hot_Start(void){
  if (MODEL.TM.timeslot != SYNC_TS)
    return;
  switch(MODEL.SYNC.mode){
    case MODE_0: break;
    case MODE_1: mode_1_process(); break;
    case MODE_2: mode_2_process(); break;
    default:
    HALT("Wrong mode");
  }
};

static struct frame* recv_sync(void){
  if(!RI_SetChannel(MODEL.SYNC.sync_channel))
    HALT("Wrong channel");
  struct frame *frame;
  
  AT_wait(SYNC_TIME - NEG_RECV_OFFSET);
  ustime_t recv_time = NWTIME_TO_US(NEG_RECV_OFFSET + POS_RECV_OFFSET);
  TRY{
    frame = RI_Receive(recv_time);
    if (!frame)
      THROW(1);
    if (frame->len != sizeof(struct sync))
      THROW(2);
    AES_StreamCoder(false, frame->payload, frame->payload, frame->len);
    return frame;
  }
  CATCH(1){
    return NULL;
  }
  CATCH(2){
    FR_delete(frame);
    return NULL;
  }
  ETRY;
  return frame;
}

static bool send_sync(void){
  struct sync sync;
  sync.sys_ch = MODEL.SYNC.sys_channel;
  sync.tx_power = MODEL.RADIO.power_tx;
  sync.panid = MODEL.SYNC.panid;
  sync.rtc = MODEL.RTC.rtc;
  sync.magic = MAGIC;
  
  struct frame *fr = FR_create();
  ASSERT(fr);
  FR_add_header(fr, &sync, sizeof(struct sync));
  
  fr->meta.SEND_TIME = (nwtime_t)SYNC_TIME;
  AES_StreamCoder(true, fr->payload, fr->payload, fr->len);
  
  bool set_ch_res = RI_SetChannel(MODEL.SYNC.sync_channel);
  ASSERT(set_ch_res);
  bool res = RI_Send(fr);
  FR_delete(fr);
  LOG_ON("SYNC sended, res = %d", res);
  return res;
}

static struct frame* network_recv_sync(ustime_t timeout){
  if(!RI_SetChannel(MODEL.SYNC.sync_channel))
    HALT("Wrong channel");
  
  struct frame *frame = NULL;
  TRY{
    frame = RI_Receive(timeout);
    if (!frame)
      THROW(1);
    if (frame->len != sizeof(struct sync))
      THROW(2);
    AES_StreamCoder(false, frame->payload, frame->payload, frame->len);
    return frame;
  }
  CATCH(1){
    return NULL;
  }
  CATCH(2){
    FR_delete(frame);
    return NULL;
  }
  ETRY;
  return frame;
}

bool network_sync(ustime_t timeout){  
  stamp_t now = UST_now();
  struct frame *frame;
  struct sync *sync;
  
  TRY{
    while(true){
      if(UST_time_over(now, timeout))
         THROW(1);
      
      frame = network_recv_sync(timeout);
      if (!frame)
         continue;
      
      sync = (struct sync*)frame->payload;
      if (sync->magic != MAGIC){
        FR_delete(frame);
        continue;
      }
     
      syncronize_timer(frame);
      
      MODEL.SYNC.synced = true;
      MODEL.SYNC.sys_channel = sync->sys_ch;
      MODEL.SYNC.panid = sync->panid;
      MODEL.RADIO.power_tx = sync->tx_power;
      MODEL.RTC.rtc = sync->rtc;
      
      retransmite = RETRANSMITE_TRY;
      MODEL.SYNC.next_time_recv = MODEL.RTC.uptime +  SEND_PERIOD ;
      MODEL.SYNC.last_time_recv = MODEL.RTC.uptime;     
      break;
    }
  }
  CATCH(1){
    FR_delete(frame);
    return false; // timeout
  }
  FINALLY{
    FR_delete(frame);
  }
  ETRY;
  return true;  
}