#include "model.h"
#include "debug.h"
#include "buffer.h"

#define NB_TABLE_ITEMS 20
#define MAX_OPEN_SLOTS 2
#define REQ_CMD  0
#define NB_FRAME_CARD_SIZE sizeof(struct frame_card)
#define NB_FRAME_REQ_SIZE sizeof(struct frame_req)
#define NB_TABLE_SIZE sizeof(struct table)

#warning GW may open lot of slots
// Шлюз имеет предопределенные временные слоты и каналы?
struct frame_card{ // Информационная карточка узла
    char ts_slots[MAX_OPEN_SLOTS]; // Временые слоты приема пакетов
    char ch_slots[MAX_OPEN_SLOTS]; // Частоты приема пакетов
    char ETX; // 0 - шлюз, 1 - одна передача до шлюза
}__attribute__((packed));

struct frame_req{ //Запрос от другого узла
  uint8_t cmd_req; // 0 - запрос информации
}__attribute__((packed));


struct table{
  unsigned int node_addr; // Адрес узла
  signed char RSSI_SIG;
  signed char LIQ;
  char weight; // Качество узла от 0 до 255. 0 - плохо
  struct frame_card card; // Информация об узле
  unsigned long update_time; // Время обновления записи
  bool record_active; // Запись действительна 
} ;

static struct table NB_TABLE[NB_TABLE_ITEMS];
static unsigned long LAST_TIME_CARD_RECIEVED;

static int find_index(struct frame *frame){
  // Попробуем найти пакет
  for (uint8_t i = 0; i < NB_TABLE_ITEMS; i++)
    if (NB_TABLE[i].node_addr == frame->meta.NSRC)
      if (NB_TABLE[i].record_active)
        return i;
  return -1;
}

/**

@brief Ищем свободное место

@return -1 

*/

static int find_free_index(){
  for (uint8_t i = 0; i < NB_TABLE_ITEMS; i++)
    if (!NB_TABLE[i].record_active)
      return i;
  return -1;
}

/**
@brief Вычисляет вес карты
@return 0 - плохо, 255 отлично
*/
static uint8_t calc_weight(struct frame_card *card){
  // Сравнивает две карты по ETX, LIQ(пока не доступно), RSSI
  // ПОДУМАТЬ. При маршрутизаии от шлюза сохраняются маршруты до соседей
  // Если нет соседа ( выкинули его) то маршрут разрушится
  return 0;
}

/**

@brief Обновление карточки по индексу

*/

static void update_record(struct frame *frame, uint8_t index){
  struct frame_card *card = (struct frame_card*)frame->payload;
  re_memcpy(&NB_TABLE[index].card, card, NB_FRAME_CARD_SIZE);
  NB_TABLE[index].update_time = MODEL.RTC.uptime;
  uint8_t weight = calc_weight(card);
  NB_TABLE[index].weight = weight;
  NB_TABLE[index].node_addr = frame->meta.NSRC;
  NB_TABLE[index].record_active = true;
  NB_TABLE[index].RSSI_SIG = frame->meta.RSSI_SIG;
  NB_TABLE[index].LIQ = frame->meta.LIQ;
}

static void insert_record(struct frame *frame){
  int index = find_free_index();
  struct frame_card *card = (struct frame_card*)frame->payload;
  uint8_t weight = calc_weight(card);
  if (index != -1){ // Нашли место, запихиваем карточку
    update_record(frame,index);
    LOG_ON("Card inserted.")
    return;
  }
  
  uint8_t bad_index, bad_weight = 255;
  bool found = false;

  // Свободных мест нет. Ищем самую плохую запись
  for (uint8_t i = 0; i < NB_TABLE_ITEMS; i++)
    if (NB_TABLE[i].record_active){
      if (NB_TABLE[i].weight < bad_weight){
        bad_weight = NB_TABLE[i].weight ;
        bad_index = i;
        found = true;
       }
    }

  if (!found){ // Если нету записей.
    LOG_ON("Card is bad. not inserted")
    return;
  }
  if (weight > bad_weight){ //// Карточка лучше чем самая плохая в таблице
    LOG_ON("Better card inserted")
    update_record(frame, bad_index);
  }
}

static bool frame_filter_card(struct frame *frame){ 
  // Фильтр 1: по размеру кадра
  if (frame->len < NB_FRAME_CARD_SIZE)
    return false;
  // struct frame_card *card = (struct frame_card*)frame->payload;
  // TODO Тут интелектуалные фильтры по содержимому
  // Желательно проверять что именно нам прислали и являются
  // ли данные корректными
  return true;
}

static bool frame_filter_cmd_req(struct frame *frame){
  // Фильтр 1: по размеру кадра
  if (frame->len < NB_FRAME_REQ_SIZE)
    return false;
  // TODO проверить доступные команды
  //struct frame_req *cmd_req = (struct frame_req*)frame->payload;
  return true;
}
/**

@brief Обработка принятой карты 

*/

static void process_card(struct frame *frame){
  int index = find_index(frame);

  if (index == -1) // Если нет записи об этом узле, вставим
    insert_record(frame);
  else{ // Если запись есть, то обновим 
    LOG_ON("Update card")
    update_record(frame, index);
  }
  // Обновим время получения последней карточки
  // мне не важно вставили или нет, главное что они регулярно приходят
  LAST_TIME_CARD_RECIEVED = MODEL.RTC.uptime;
}

static void send_card(void){
  struct frame_card card;

  int etx = NP_GetETX();
  
  // Выходим если ETX не определен
  if (etx == -1){
    // Раз нет ETX то продлим время отправки карты
    uint32_t now = MODEL.RTC.uptime();
    NEXT_CARD_SEND_TIME = now + NEIGHBOR_CARD_SEND_INTERVAL + 
      rand() % NEIGHBOR_CARD_SEND_INTERVAL_DEV;
    LOG_ON("ETX not defind. Card not sended.")
    return;
  }

  card.ETX = etx;

  // Проверим что у нас есть открытые слоты
  bool opened = false;
  for (uint8_t i = 0; i < MAX_OPEN_SLOTS; i++){
  if (CONFIG.ts_slots[i] !=0 )
    if (CONFIG.ch_slots[i] !=0){
      opened = true;
      break;
    }
  }

 
  if (!opened){
    uint32_t now = TIC_GetUptime();
    NEXT_CARD_SEND_TIME = now + NEIGHBOR_CARD_SEND_INTERVAL + 
      rand() % NEIGHBOR_CARD_SEND_INTERVAL_DEV;
    LOG_ON("Slots not opened. Card not sended.")
    return;
  }

  for (uint8_t i = 0; i < MAX_OPEN_SLOTS; i++){
    card.ts_slots[i] = CONFIG.ts_slots[i];
    card.ch_slots[i] = CONFIG.ch_slots[i];
  }

  frame_s *fr = frame_create();
  frame_addHeader(fr, &card, NB_FRAME_CARD_SIZE);
  fr->meta.PID = PID_NP;
  fr->meta.NDST = 0xffff;
  fr->meta.NSRC = CONFIG.node_adr; 
  fr->meta.TS = 0;
  fr->meta.CH = CONFIG.sys_channel;
  fr->meta.TX_METHOD = BROADCAST;
  RP_Send(fr);
  LOG_ON("NP Card sended");
}


/**
@brief Обработка принятой команды
*/
static void process_cmd_req(struct frame *frame){
  struct frame_req *cmd_req = (struct frame_req*)frame->payload;  

  // Запрос информации об узле
  if (cmd_req->cmd_req == REQ_CMD)
    send_card();
  LOG_ON("CMD reques processed.")
}

void NP_Receive(struct frame *frame){
  if (frame->meta.NSRC == 0xffff)
    return;
  
  if (frame_filter_card(frame))
      process_card(frame);
  else if (frame_filter_cmd_req(frame))
      process_cmd_req(frame);
}