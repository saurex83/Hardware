#include "model.h"
#include "debug.h"
#include "protocol_defs.h"
#include "ip.h"
#include "neigh.h"
#include "balancer.h"
#include "auth_eth.h"
#include "ethernet.h"
#include "mem_utils.h"

#define ROUTE_TABLE_ITEMS 20
#define ROUTE_LIVE_TIME 15 // Время жизни маршрута в минутах

struct route_record{
  unsigned int nsrc;
  unsigned int fsrc;
  char live_time;
  char NSRC_TS;
  char NSRC_CH;
  bool RA;
} __attribute__((packed));

static struct route_record ROUTE_TABLE[ROUTE_TABLE_ITEMS];

void RP_Init(){
  // Очистим таблицу соседей
  MEMSET((char*)ROUTE_TABLE, 0, sizeof(ROUTE_TABLE));
};

static void analyse_route_table(){
  static unsigned long last_analyse_time = 0;
  unsigned long now = MODEL.RTC.uptime;
  
  // Ждем что пройдем как минимум 1 минута с последнего анализа
  if ((now - last_analyse_time) < 60)
    return;
  
  last_analyse_time = now;
  
  LOG_ON("Analyse ROUTE table");
  
  // Теперь уменьшим время жизни записям и удалим при необходимости
  for (int  i = 0; i < ROUTE_TABLE_ITEMS; i++){
    // Пропускаем пустые записи
    if (!ROUTE_TABLE[i].RA)
      continue;
    
    // Уменьшаем время жизни
    if (ROUTE_TABLE[i].live_time > 0)
      ROUTE_TABLE[i].live_time--;
    
    // Помечаем запись свободной по истечению времени жизни
    if (ROUTE_TABLE[i].live_time == 0){
      LOG_ON("Record ROUTE: nsrc %d, fdst %d deleted", 
             ROUTE_TABLE[i].nsrc, ROUTE_TABLE[i].fsrc);
      ROUTE_TABLE[i].RA = false;    
    }
  };
};

void RP_TimeAlloc(){
  analyse_route_table();
};

void RP_Receive(struct frame *frame){
  #warning update route tables
  switch (frame->meta.PID){
  case PID_IP: // Пакет относится к протоколу IP
    IP_Receive(frame);
    break;
  case PID_NP: // Пакет относится к протоколу соседей
    NP_Receive(frame);
    break;
  case PID_AUTH: // Пакет относится к протоколу авторизации
    AUTH_ETH_Receive(frame);
    break;
  default:
    return; // Пакет удалит ethernet
  };
  
  TB_Receive(frame);  
};

/** brief Отправить пакет лучшему соседу
*/
void RP_Send_COMM(struct frame *frame){
  bool condition = MODEL.SYNC.synced && MODEL.NEIGH.comm_node_found;
  if (!condition){
    FR_delete(frame);
    LOG_ON("Cant send.");
    return;
  };  
  
  char ts;
  char ch;
  unsigned int addr;
  bool res = comm_node_info(&addr, &ts, &ch);
  if (!res){
    FR_delete(frame);
    LOG_ON("Problem with comm_node_info");
  };
  
  frame->meta.TS = ts;
  frame->meta.CH = ch;
  frame->meta.NDST = addr;
  frame->meta.NSRC = MODEL.node_adr;
  eth_send(frame);
  LOG_ON("Sended to COMM node. NDST=%d, CH=%d, TS=%d", addr, ch, ts);  
};

void RP_Send_GW(struct frame *frame){
  bool condition = MODEL.AUTH.auth_ok && MODEL.AUTH.access_ok &&
    MODEL.SYNC.synced && MODEL.NEIGH.comm_node_found;
  if (!condition){
    FR_delete(frame);
    LOG_ON("Cant send.");
    return;
  };
  
  char ts;
  char ch;
  unsigned int addr;
  bool res = comm_node_info(&addr, &ts, &ch);
  if (!res){
    FR_delete(frame);
    LOG_ON("Problem with comm_node_info");
  };
  
  frame->meta.TS = ts;
  frame->meta.CH = ch;
  frame->meta.NDST = addr;
  frame->meta.NSRC = MODEL.node_adr;
  LOG_ON("Sended to GW. frame_ptr=0x%x, NDST=%d, NSRC=%d, FDST=%d, FSRC=%d, TS=%d, CH=%d",
         (char*)frame,frame->meta.NDST,frame->meta.NSRC,
         frame->meta.FDST,frame->meta.FSRC,ts,ch);
  eth_send(frame);
  
};

void RP_Send(struct frame *frame){
  eth_send(frame);
  LOG_ON("Sended raw");  
};

/** brief Ищем номер записи по паре значений
*/
static int route_find(unsigned int nsrc, unsigned int fsrc){
  for (int i = 0; i < ROUTE_TABLE_ITEMS; i++)
    if (ROUTE_TABLE[i].nsrc == nsrc && ROUTE_TABLE[i].fsrc == fsrc
        && ROUTE_TABLE[i].RA)
      return i;
  return -1;
};

static int find_free(){
  for (int i = 0; i < ROUTE_TABLE_ITEMS; i++)
    if (!ROUTE_TABLE[i].RA)
      return i;
  return -1;  
};

static bool route_update(unsigned int nsrc, unsigned int fsrc,
                         char nsrc_ts, char nsrc_ch){
  // Ищем запись о маршруте
  int idx = route_find(nsrc, fsrc);
  
  // Если такого маршрут небыло, ищем свободную ячейку
  if (idx < 0)
    idx = find_free();
  
  // Если таблица пуста выходим
  if (idx < 0){
    LOG_ON("Route table full!");
    return false;
  };
  
  ROUTE_TABLE[idx].nsrc = nsrc;
  ROUTE_TABLE[idx].fsrc = fsrc;
  ROUTE_TABLE[idx].live_time= ROUTE_LIVE_TIME;
  ROUTE_TABLE[idx].NSRC_TS = nsrc_ts;
  ROUTE_TABLE[idx].NSRC_CH = nsrc_ch;
  ROUTE_TABLE[idx].RA = true;  
  return true;
};

/** Пересылка пакета к шлюзу с добавлением маршрута
*/
void RP_SendRT_GW(struct frame *frame){
  // Информация как был получен пакет и от кого
  unsigned int nsrc = frame->meta.NSRC;
  unsigned int fsrc = frame->meta.FSRC;
  char nsrc_ts = frame->meta.NSRC_TS;
  char nsrc_ch = frame->meta.NSRC_CH;   
  
  // Обновим таблицу маршрутов
  bool res = route_update(nsrc, fsrc, nsrc_ts, nsrc_ch);
  if (res){
    LOG_ON("Route table updated. nsrc=%d, fsrc=%d, ts=%d, ch=%d",
           nsrc, fsrc, nsrc_ts, nsrc_ch);
  } else
    LOG_ON("Route table update faild. nsrc=%d, fsrc=%d, ts=%d, ch=%d",
         nsrc, fsrc, nsrc_ts, nsrc_ch);
  
  
  // frame будет удален после выхода из eth_receive и пакет не будет 
  // отправлен. Нужно его скопировать и только потом отправлять
  struct frame *new_frame = FR_create();
  ASSERT(new_frame);
  
  FR_add_header(new_frame, frame->payload, frame->len);
  new_frame->meta.FSRC = fsrc;
  new_frame->meta.PID = frame->meta.PID;
  new_frame->meta.IPP = frame->meta.IPP;
  
  RP_Send_GW(new_frame);
};

/** brief Ищем маршрут до fdst
*/
static int route_find_by_fsrc(unsigned int fsrc){
  for (int i = 0; i < ROUTE_TABLE_ITEMS; i++)
    if (ROUTE_TABLE[i].fsrc == fsrc && ROUTE_TABLE[i].RA)
      return i;
  return -1;
};

/** brief Передача от шлюза к какому то узлу
*/
void RP_SendRT_RT(struct frame *frame){
  unsigned int fsrc = frame->meta.FSRC;
  unsigned int fdst = frame->meta.FDST;
  
  if (fsrc != 0){
    LOG_ON("Frame must be sended GW. Drop. fsrc=%d", fsrc);
    return;
  };
  
  if (fdst == 0){
    LOG_ON("Frame fdst is 0. Drop.");
    return;
  };
  
  if (fdst == 0xffff){
    LOG_ON("Frame fdst is 0xffff. Drop.");
    return;
  };
  
  // Ищем через кого раньше был отправлен пакет
  int idx = route_find_by_fsrc(fdst);
  if (idx < 0){
    LOG_ON("Route to node %d not found. Drop.", fdst);
    return;
  };
  
  // frame будет удален после выхода из eth_receive и пакет не будет 
  // отправлен. Нужно его скопировать и только потом отправлять
  struct frame *new_frame = FR_create();
  ASSERT(new_frame);
  
  FR_add_header(new_frame, frame->payload, frame->len);

  new_frame->meta.PID = frame->meta.PID; 
  new_frame->meta.TS = ROUTE_TABLE[idx].NSRC_TS;
  new_frame->meta.CH = ROUTE_TABLE[idx].NSRC_CH;
  new_frame->meta.NDST = ROUTE_TABLE[idx].nsrc;
  new_frame->meta.NSRC = MODEL.node_adr;
  
  
  LOG_ON("Route frame(fdst=%d, fsrc=%d) to NODE %d by route table. ", 
    fdst, fsrc, ROUTE_TABLE[idx].nsrc); 
    
  eth_send(new_frame);

};