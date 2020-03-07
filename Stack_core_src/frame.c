#include "action_manager.h"
#include "frame.h"
#include "debug.h"
#include "global.h"
#include "mem_utils.h"
#include "mem_slots.h"

/**
@file Статическое хранение принятых пакетов
*/

static void SW_Init(void); 

module_s FR_MODULE = {ALIAS(SW_Init)};

static void SW_Init(void){ 
}; 

struct frame* FR_create(){
  return (struct frame*)SL_alloc();
};

bool FR_delete(struct frame *frame){
  return SL_free((char*)frame);
}

bool FR_add_header(struct frame* frame ,void *head, char len){
  int new_len = frame->len + len;
  if (!(new_len < MAX_PAYLOAD_SIZE))
    return false;
  
  // Сдвинем данные на размер вставки при необходимости
  if (frame->len != 0)
    MEMCPY(&frame->payload[len], frame->payload, len);
  
  // Скопируем новые данные
  MEMCPY(frame->payload, head, len);
  frame->len = new_len;
  return true;
};

bool FR_del_header(struct frame* frame, char len){
  if (len == 0 || len > frame->len )
    return false;
  MEMCPY(frame->payload, &frame->payload[len], len);
  
  #ifdef FRAME_FOOTER_DEL
  MEMSET(&frame->payload[len], 0, len);
  #endif
  
  frame->len = frame->len - len;;
  return true;
}

int FR_busy(){
  return SL_busy();
}

int FR_available(){
  return SL_available();
};