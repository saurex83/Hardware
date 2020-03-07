#include "model.h"
#include "debug.h"
#include "buffer.h"
#include "route.c"

#define ETH_LAY_SIZE sizeof(struct ETH_LAY)

struct ETH_LAY{  
  union{
    uint8_t value;
    struct {
      char PID:4;
      char UNUSED:1;
      char ETH_VER:3;
    } bits;
  } ETH_T;

  char NETID;
  unsigned int NDST;
  unsigned int NSRC;
} __attribute__((packed));

static bool frame_filter(struct frame *frame){
  struct ETH_LAY *eth_header = (struct ETH_LAY*)frame->payload;

  // Фильтр 1: по размеру кадра
  if (frame->len < ETH_LAY_SIZE)
    return false;

  // Фильтр 2: по версии протокола
  if (eth_header->ETH_T.bits.ETH_VER != HEADER_ETH_VER)
    return false;

  // Фильтр 3: по идентификатору сети
  if (eth_header->NETID!= MODEL.SYNC.panid)
    return false;

  // Фильтр 4: по адресу получателю
  if (eth_header->NDST != 0xffff )
    if (eth_header->NDST != MODEL.node_adr)
      return false;
  return true;
}

static inline void fill_meta_data(struct frame *frame){
  // Заполняем метаданные
  struct ETH_LAY *eth_header = (struct ETH_LAY*)frame->payload;
  frame->meta.NDST = eth_header->NDST;
  frame->meta.NSRC = eth_header->NSRC;
  frame->meta.PID = eth_header->ETH_T.bits.PID;  
}

static void parse_frame(struct frame *frame){
  // Разбор пакета
  if (!frame_filter(frame))
    return;
  
  fill_meta_data(frame);
  FR_del_header(frame, ETH_LAY_SIZE);
  RP_Receive(frame);  
}

void ethernet_process(void){
  // Извлекает все принятые пакеты из буфера
  void* cursor = BF_cursor_rx();
  void *cursor_delete;
  
  struct frame *rx_frame;
  
  while (cursor){
    rx_frame = BF_content(cursor);
    parse_frame(rx_frame);
    
    if (!FR_delete(rx_frame))
      HALT("Delete");
    
    cursor_delete = cursor;
    if (!BF_remove_rx(cursor_delete))
      HALT("Remove");
    
    cursor = BF_cursor_next(cursor);
  }
  if (!BF_remove_rx(cursor))
    HALT("Remove error");  
};