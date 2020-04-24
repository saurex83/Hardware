#include "model.h"
#include "debug.h"
#include "route.h"
#include "auth_eth.h"
#include "neigh.h"
#include "llc.h"

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
  char NSRC_TS;
  char NSRC_CH;
  char XOR;
} __attribute__((packed));

static char calc_xor(struct ETH_LAY *eth){
  char xor_val = 0x57;
  char *ptr = (char*)eth;
  
  // Ксорим все кроме последнего байта структуы. он XOR
  for (int i = 0; i < sizeof(struct ETH_LAY) - 1; i++)
    xor_val ^=ptr[i];
  
  return xor_val;
};

static bool frame_filter(struct frame *frame){
  struct ETH_LAY *eth_header = (struct ETH_LAY*)frame->payload;
  
  // Фильтр 0: по размеру кадра
  if (frame->len < ETH_LAY_SIZE){
    LOG_ON("Filtered ETH_LAY_SIZE");
    return false;
  }
  
  // Фильтр 1: по XOR
  char xor_val = calc_xor(eth_header);
  if (xor_val != eth_header->XOR){
    LOG_ON("Filtered XOR");
    return false;
  };
  
  // Фильтр 2: по версии протокола
  if (eth_header->ETH_T.bits.ETH_VER != HEADER_ETH_VER){
    LOG_ON("Filtered ETH_VER. %d",eth_header->ETH_T.bits.ETH_VER );
    return false;
  }

  // Фильтр 3: по идентификатору сети
  if (eth_header->NETID!= MODEL.SYNC.panid){
    LOG_ON("Filtered panid");
    return false;
  }
  
  bool from_gw = (eth_header->NSRC_CH == 0xff) &&
                 (eth_header->NSRC_TS == 0xff);
  if (!from_gw){
    // Фильтр 4: по каналу отправителя
    if (eth_header->NSRC_CH  < CH11 || eth_header->NSRC_CH  > CH28){
      LOG_ON("Filtered NSRC_CH");
      return false;
    }
    // Фильтр 5: по таймслоту отправителя
    if (eth_header->NSRC_TS  < 1 || eth_header->NSRC_TS  > 49){
      LOG_ON("Filtered NSRC_TS");
      return false;
    }
  };
  
  // Фильтр 6: по адресу получателю
  if (eth_header->NDST != 0xffff )
    if (eth_header->NDST != MODEL.node_adr){
      LOG_ON("Filtered node addr");
      return false;
    }
  return true;
}

static inline void fill_meta_data(struct frame *frame){
  // Заполняем метаданные
  struct ETH_LAY *eth_header = (struct ETH_LAY*)frame->payload;
  frame->meta.NDST = eth_header->NDST;
  frame->meta.NSRC = eth_header->NSRC;
  frame->meta.PID = eth_header->ETH_T.bits.PID;  
  frame->meta.NSRC_TS = eth_header->NSRC_TS;
  frame->meta.NSRC_CH = eth_header->NSRC_CH;
}

static void parse_frame(struct frame *frame){
  // Разбор пакета
  LOG_ON("Frame Filter");
  if (!frame_filter(frame))
    return;
  
  LOG_ON("Fill metadata");
  fill_meta_data(frame);
  LOG_ON("Delete eth header");
  FR_del_header(frame, ETH_LAY_SIZE);
  LOG_ON("Route protocol");
  RP_Receive(frame);  
}

/* brief Обработка принятых пакетов
* Извлекает пакеты из входящего буфера, разбирает заголовок ETH, фильтрует 
* пакет и передает их на обработку  протоколу маршрутизации. 
* После обработки пакет удаляется.
*/
void ethernet_process(void){
  struct frame* frame = NULL;
  frame = FR_find_rx(frame); 
  LOG_OFF("Start search rx");
  while (frame){
    LOG_ON("Find rx!");
    LOG_ON("ETH. LEN:%d, TS:%d, CH:%d, PID:%d", frame->len,
           frame->meta.TS, frame->meta.CH, frame->meta.PID);
    parse_frame(frame);
    FR_delete(frame);
    frame = FR_find_rx(frame);
  };
  LOG_OFF("Stop search rx");
  
  // Выделение свободного времени
  AUTH_ETH_TimeAlloc();
  NP_TimeAlloc();
  RP_TimeAlloc();
};

void eth_send(struct frame *frame){
  struct ETH_LAY eth_header;
  eth_header.ETH_T.bits.PID = frame->meta.PID ;
  eth_header.ETH_T.bits.ETH_VER =HEADER_ETH_VER;
  eth_header.NETID = MODEL.SYNC.panid;
  eth_header.NDST = frame->meta.NDST;
  eth_header.NSRC = frame->meta.NSRC;
  eth_header.NSRC_CH = MODEL.node_CH;
  eth_header.NSRC_TS = MODEL.node_TS;
  eth_header.XOR = calc_xor(&eth_header);
  
  bool res;
  res = FR_add_header(frame, &eth_header,
                      sizeof(struct ETH_LAY));
  ASSERT(res);
 
  frame->meta.tx_attempts = 5;
  res = LLC_add_tx_frame(frame); 
};
