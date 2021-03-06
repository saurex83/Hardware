#include "model.h"
#include "debug.h"
#include "protocol_defs.h"
#include "ethernet.h"
#include "stdlib.h"
#include "mem_utils.h"

#define NB_ITEMS 10
#define TIME_TO_LIVE 10 // в минутах
#define CARD_SEND_PERIOD 60 // в секундах
#define COMM_NODE_UPDATE_PERIOD 60//в секундах
#define CRIT_WEIGHT 10  // Критический вес узла для смены
#define PERIOD_REQ_TIME 120 // Интервал запроса карточек

struct gw_vec{
  char ts;
  char ch;
};

#define GW_VECTOR_SIZE 10
static const struct gw_vec GW_VECTOR[GW_VECTOR_SIZE] = {
  {49, CH11}, {48, CH13}, {47, CH15}, {46, CH17}, {45, CH19},
  {44, CH20}, {43, CH21}, {42, CH22}, {41, CH23}, {40, CH25}
};

struct np_record {
  char LIVE_TIME; //Время в минутах
  bool RA;
  signed char RSSI_SIG;
  char LIQ;
  char TS_SLOT;
  char CH_SLOT;
  char ETX;
  unsigned int ADDR;
};

struct NP_CARD{
  char TS;
  char CH;
  char ETX;
} __attribute__((packed));

enum {NP_CMD_CARD = 0, NP_CMD_CARD_REQ = 1} NP_CMD_ENUM;

static struct np_record NB_TABLE[NB_ITEMS];
// Указатель на лучший узел для связи
static struct np_record *COMM_NODE_PTR = NULL;
// Время последнего обновления указателя
static unsigned long LAST_UPDATE_TIME_COMM_NODE = 0;


void NP_Init(){
  // Очистим таблицу соседей
  MEMSET((char*)NB_TABLE, 0, sizeof(NB_TABLE));
  COMM_NODE_PTR = NULL;
  LAST_UPDATE_TIME_COMM_NODE = 0;
  MODEL.NEIGH.comm_node_found = false;
};

/** brief Возращает информацио об узле связи с шлюзом
*/
bool comm_node_info(unsigned int *addr, char *ts, char *ch){
  if (!COMM_NODE_PTR)
    return false;
  
  // Если узлом связи является шлюз, то у него другой алгоритм выбора TS и CH
  if (COMM_NODE_PTR->ADDR == 0){
    char idx = rand()%GW_VECTOR_SIZE;
    *addr = 0;
    *ts = GW_VECTOR[idx].ts;
    *ch = GW_VECTOR[idx].ch;
    return true;
  };
  
  *addr = COMM_NODE_PTR->ADDR;
  *ts = COMM_NODE_PTR->TS_SLOT;
  *ch = COMM_NODE_PTR->CH_SLOT;
  return true;
};

/** brief Отправим карточку узла
*/
static void send_node_card(){
  // Мы не может послать карту так как не знаем свой ETX
  LOG_ON("Send node card");
  if (!COMM_NODE_PTR)
    return;
    
  struct NP_CARD card;
  card.TS = MODEL.node_TS;
  card.CH = MODEL.node_CH;
  card.ETX = MODEL.node_ETX;
  
  struct frame *fr = FR_create();
  bool res =FR_add_header(fr, (char*)&card, sizeof(struct NP_CARD));
  char cmd = NP_CMD_CARD;
  res = FR_add_header(fr, &cmd, sizeof(cmd));
  
  fr->meta.TS = 1;
  fr->meta.CH = MODEL.SYNC.sys_channel;
  fr->meta.NDST = 0xffff;
  fr->meta.NSRC = MODEL.node_adr;
  fr->meta.PID = PID_NP;
  eth_send(fr);
  LOG_ON("Card sended");
};

static int index_by_addr(unsigned int addr){
  int ret = -1;
  for (int  i = 0; i < NB_ITEMS; i++)
    if (NB_TABLE[i].RA && NB_TABLE[i].ADDR == addr){
      ret = i;
      break;
    };
  return ret;
};

static int free_index(){
  int ret = -1;
  for (int  i = 0; i < NB_ITEMS; i++)
    if (!NB_TABLE[i].RA){
      ret = i;
      break;
    };
  return ret;
};

/** brief Добавим или обновим карточку в таблице
* Уничтожения худших карт не происходит
* Добавляются соседи с RSSI >= -90
*/
static void add_card(struct frame *frame){
  LOG_ON("Add card");
  if (frame->len != sizeof(struct NP_CARD))
    return;
  
  if (frame->meta.RSSI_SIG < -90){
    LOG_ON("Bad frame rssi=%d", frame->meta.RSSI_SIG);
      return;
  };

  
  struct NP_CARD *card = (struct NP_CARD*)frame->payload;
  // Проверим присланные параметры
  bool filter = true;
  filter &= card->TS >= 2 && card->TS <= 49;
  filter &= card->CH >=CH11 && card->CH <= CH27;
  filter &= card->ETX < 250;
  if (!filter){
    LOG_ON("Card filtered.exit");
    return;
  }
  
  int idx;
  
  // Ищем запись по адресу или свободный индекс
  idx = index_by_addr(frame->meta.NSRC);
  if (idx >= 0){
    LOG_ON("Card found by addr. idx = %d", idx);
  };
  
  if (idx < 0){
    idx = free_index();
    if (idx >=0 ){
      LOG_ON("Card found by free index %d",idx);
    };
  };
 
  // Не получиться вставить запись
  if (idx < 0){
    LOG_ON("Add card exit. no free index");
    return;
  }
  
  // Обнавляем таблицу
  NB_TABLE[idx].LIVE_TIME = TIME_TO_LIVE;
  NB_TABLE[idx].ADDR = frame->meta.NSRC;
  NB_TABLE[idx].RA = true;
  NB_TABLE[idx].RSSI_SIG = frame->meta.RSSI_SIG;
  NB_TABLE[idx].LIQ = frame->meta.LIQ;
  NB_TABLE[idx].TS_SLOT = card->TS;
  NB_TABLE[idx].CH_SLOT = card->CH;
  NB_TABLE[idx].ETX = card->ETX;
  
  LOG_ON("Card added! ADDR=%d, RSSI=%d, ETX=%d", 
         NB_TABLE[idx].ADDR,NB_TABLE[idx].RSSI_SIG, NB_TABLE[idx].ETX);
};

void NP_Receive(struct frame *frame){
  LOG_ON("NP Receive");
  // Если ущел не синхронизирован выходим
  if (!MODEL.SYNC.synced){
    LOG_ON("Node unsinced exit");
    return;
  }
  
  // Если нам отказали в доступе
  if (MODEL.AUTH.auth_ok && !MODEL.AUTH.access_ok){
    LOG_ON("Access depricated");
    return;
  }
  
  if (frame->len == 0){
    LOG_ON("Wrong size");
    return;
  }
  
  // Извлечем номер команды
  char cmd = frame->payload[0];
  bool res = FR_del_header(frame, 1);
  ASSERT(res);
  
  bool all_ok = MODEL.AUTH.auth_ok && MODEL.AUTH.access_ok;
  switch (cmd){
    case NP_CMD_CARD:
      add_card(frame);
      return;
    case NP_CMD_CARD_REQ:
      if (!all_ok)
        return;
      send_node_card();
      break;
    default:
    LOG_ON("Invalid NP_CMD");
  };
}

/** brief Периодическа рассылка карты узла
*/
static void periodic_card_send(){
  static unsigned long last_card_send_time = 0;
  unsigned long now = MODEL.RTC.uptime;
  
  // Ждем что пройдем как завершения периода рассылки
  if ((now - last_card_send_time) < CARD_SEND_PERIOD)
    return;
  
  LOG_ON("Periodic send");
  last_card_send_time = now;
  send_node_card();
};

/** brief Анализ таблицы и уменьшении времени жизни записи
* TODO выбрасывать плохих соседей с высоким ETX
*/
static void analyse_table(){
  static unsigned long last_analyse_time = 0;
  unsigned long now = MODEL.RTC.uptime;
  
  // Ждем что пройдем как минимум 1 минута с последнего анализа
  if ((now - last_analyse_time) < 60)
    return;
  
  last_analyse_time = now;
  
  LOG_ON("Analyse NP table");
  
  // Теперь уменьшим время жизни записям и удалим при необходимости
  for (int  i = 0; i < NB_ITEMS; i++){
    // Пропускаем пустые записи
    if (!NB_TABLE[i].RA)
      continue;
    
    // Уменьшаем время жизни
    if (NB_TABLE[i].LIVE_TIME > 0)
      NB_TABLE[i].LIVE_TIME--;
    
    // Помечаем запись свободной по истечению времени жизни
    if (NB_TABLE[i].LIVE_TIME == 0){
      LOG_ON("Record NODE: %d deleted", NB_TABLE[i].ADDR);
      NB_TABLE[i].RA = false;    
    }
  };
};

/** brief Поиск узла с минимальным ETX(приоритет) и максимальным RSSI
* 
*/
static int find_node(){
  int idx = -1;
  signed char rssi = -90;
  char etx = 255;
  
  for (int  i = 0; i < NB_ITEMS; i++){
    // Пропускаем пустые записи
    if (!NB_TABLE[i].RA)
      continue;
  
    // Если ETX ниже то беззоговорочно выбираем этот узел
    if (NB_TABLE[i].ETX < etx){
      rssi = NB_TABLE[i].RSSI_SIG;
      etx = NB_TABLE[i].ETX;
      idx = i;
    };
    
    // Среди соседий с низким ETX выбираем того у кого лучше сигнал
    if (NB_TABLE[i].ETX == etx)
      if (NB_TABLE[i].RSSI_SIG > rssi){
        rssi = NB_TABLE[i].RSSI_SIG;
        etx = NB_TABLE[i].ETX;
        idx = i;
      };
  };
  
  // Раскажем что мы нашли
  if (idx >= 0)
    LOG_ON("Find: idx=%d, addr=%d, rssi=%d, etx=%d",
           idx, NB_TABLE[idx].ADDR, NB_TABLE[idx].RSSI_SIG,
           NB_TABLE[idx].ETX);
    
  return idx;
};

/** brief Выбираем и следим за указателем на узел связи с шлюзом
*/
static void comm_node_choise(){
  unsigned long now = MODEL.RTC.uptime;
  
  // Проверим активна запись на которую указывает
  if (COMM_NODE_PTR)
    if (!COMM_NODE_PTR->RA){
      LOG_ON("COMM_NODE_PTR no active. reset to null");
      COMM_NODE_PTR = NULL;
      MODEL.NEIGH.comm_node_found = false;
    }
  
  // Если узел уже выбран, то обнавляем с некоторым периодом
  if (COMM_NODE_PTR){ 
    if ((now - LAST_UPDATE_TIME_COMM_NODE) < COMM_NODE_UPDATE_PERIOD)
      return;
    LAST_UPDATE_TIME_COMM_NODE = now;
  };
  
  
  // Есть ли у нас хоть один сосед
  bool table_free = true;
  for (int  i = 0; i < NB_ITEMS; i++)
    if (NB_TABLE[i].RA){
      table_free = false;
      break;
    };
 
  // Заканчиваем работу если соседей нет
  if (table_free){
    LOG_OFF("Dont have neig. exit");
    return;
  }
  
  // Начинаем выбирать соседей для связи
  int idx = find_node();
  if (idx < 0){
    LOG_ON("Imposible");  
    return; // Этого быть не должно
  };
  
  // Нашли тот же самый узел
  // Нужно перерасчитать ETX так как найденый узел мог обновить свой маршут
  if (COMM_NODE_PTR == &NB_TABLE[idx]){
    LAST_UPDATE_TIME_COMM_NODE = now;
    MODEL.node_ETX = COMM_NODE_PTR->ETX + 1;  
    LOG_ON("Unchanged IDX=%d. PTR=%d, ADDR", idx, &NB_TABLE[idx], NB_TABLE[idx].ADDR);
    return;
  };
  
  // Меняем узел
  COMM_NODE_PTR = &NB_TABLE[idx];
  LAST_UPDATE_TIME_COMM_NODE = now;
  MODEL.node_ETX = COMM_NODE_PTR->ETX + 1;  
  MODEL.NEIGH.comm_node_found = true;
  LOG_ON("Change. Find IDX=%d. PTR=%d, ADDR", idx, &NB_TABLE[idx], NB_TABLE[idx].ADDR);    
};

static bool is_free(){
  for (int  i = 0; i < NB_ITEMS; i++)
    if (NB_TABLE[i].RA)
      return false;
    return true;
};


static void request_cards(){
  // Запрашиваем карты если их нет совсем
  // Мы не может послать карту так как не знаем свой ETX 
  if (!is_free()){
    LOG_OFF("Table not free.exit");
    return;
  }

  LOG_OFF("Start request card");
  
  static unsigned long last_card_req_time = 0;
  unsigned long now = MODEL.RTC.uptime;
  
  // Проверим что прошло достаточно времени
  if ((now - last_card_req_time) < PERIOD_REQ_TIME)
    return;
  
  last_card_req_time = now;
  
  LOG_ON("REQUEST NP CARD");
  
  struct frame *fr = FR_create();
  ASSERT(fr);
  char cmd = NP_CMD_CARD_REQ;
  bool res =FR_add_header(fr, &cmd, sizeof(cmd));
  ASSERT(res);
  
  fr->meta.NDST = 0xffff;
  fr->meta.NSRC = MODEL.node_adr;
  fr->meta.PID = PID_NP;
  eth_send(fr);
  LOG_ON("Card requested");
};

void NP_TimeAlloc(){
  // Если узел не синхронизирован выходим
  if (!MODEL.SYNC.synced)
    return;
  
  // Если мы прошли авторизацию и получили доступ
  if (MODEL.AUTH.auth_ok && MODEL.AUTH.access_ok){
    LOG_OFF("Auth passed access granted");
    periodic_card_send();
    analyse_table();
    request_cards();
    comm_node_choise();
    return;
  }
  
  // Если мы не прошли процедуру авторизации
  if (!MODEL.AUTH.auth_ok){
    LOG_OFF("Auth not passed");
    analyse_table();
    request_cards();
    comm_node_choise();
    return;
  };
  
  // Если нам отказали в доступе
  if (MODEL.AUTH.auth_ok && !MODEL.AUTH.access_ok){
    LOG_ON("Auth passed. Access depricated");
    return;
  }
};