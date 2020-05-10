#include "model.h"
#include "debug.h"
#include "frame.h"
#include "ethernet.h"
#include "protocol_defs.h"
#include "stdlib.h"
#include "llc.h"
#include "route.h"
#include "mem_utils.h"

#define REQUEST_INTERVAL 30
#define AUTH_RESP_GW_INTERVAL 5

static unsigned long last_request = 0;

static void AUTH_request();
static void receiveCMD_Request(struct frame* frame);
static void receiveCMD_Response(struct frame* frame);

enum AUTH_ETH_TYPES {AUTH_CMD_REQ = 0, AUTH_CMD_RESP = 1};

struct AUTH_ETH_REQ{
  char mac[8]; // Кто делает запрос
  char node_type;
  char node_ver;
  char sensor_types[7];
  char sensor_channels[7];
} __attribute__((packed));

struct AUTH_ETH_RESP{
  char mac[8]; // Для кого предназначен ответ
  bool access;
  unsigned int ipaddr;
  char param[16];
} __attribute__((packed));

struct auth_req{
  bool RA;
  struct AUTH_ETH_REQ req;
};

struct auth_res{
  bool RA;
  struct AUTH_ETH_RESP res;
};

// Таблица хранит запросы от других узлов на подключение
#define AUTH_NODE_REQ_SIZE 5
static struct auth_req AUTH_NODE_REQ[5];

// Таблица хранит ответы шлюза на подключения для других узлов
#define AUTH_NODE_RESP_SIZE 5
static struct auth_res AUTH_NODE_RESP[5];

void AUTH_ETH_Init(){
  MEMSET((char*)AUTH_NODE_RESP, 0, sizeof(AUTH_NODE_RESP));
  MEMSET((char*)AUTH_NODE_REQ, 0, sizeof(AUTH_NODE_REQ));
};

void AUTH_ETH_Receive(struct frame *frame){
  LOG_ON("AUTH_ETH receive frame");
  
  // Всеравно откого получили пакет. в пакет вклчен мак адрес
  bool filter_rule = frame->len > 0;  
  if (!filter_rule){
    LOG_ON("AUTH_ETH frame filltered");
    return;
  }
  
  char cmd = frame->payload[0];
  bool res = FR_del_header(frame, 1);
  ASSERT(res);
  
  switch(cmd){
    case AUTH_CMD_REQ :
      // TODO обработка приема
      receiveCMD_Request(frame);
      break;
  case AUTH_CMD_RESP:
      // TODO обработка приема 
      receiveCMD_Response(frame);
      break;
  default:
    LOG_ON("Unrecognized cmd");
    break;
  };
};

/** brief Выбирает и устанавлиет канал коммуникации для узла
*
* Частотный канал  [CH11..CH27] выбор из 17 каналов
* Временой канал [2..39] выбор из 38 временых слотов
*/
static void setNodeCHTS(){
  char ch = 11 + rand()%17;
  char ts = 2 + rand()%38;
  MODEL.node_TS = ts;
  MODEL.node_CH = ch;
  LLC_open_slot(ts, ch);
  LOG_ON("Node choose TS=%d, CH=%d", ts, ch);
};

/** brief Обработка ответа об авторизации
* Ответ принимается если узел еще не авторизован и пакет
* предназначени этому узлу
*/
static void receiveCMD_Response(struct frame* frame){
  LOG_ON("AUTH_ETH Response");
  if (MODEL.AUTH.auth_ok == true)
    return;  
  
  if (frame-> len != sizeof(struct AUTH_ETH_RESP)){
    LOG_ON("Frame AUTH_ETH_RESP wrong size");
    return;
  };
  LOG_ON("Extract response");
  struct AUTH_ETH_RESP *resp = (struct AUTH_ETH_RESP*)frame->payload;
  
  // Проверим что пакет для нас. Должны совпадать мак адресса
  bool mac_eq = true;
  for (char i = 0; i < 8; i++)
    if (resp->mac[i] != MODEL.node_mac[i]){
      mac_eq = false;
      break;
    };
  
  if (!mac_eq){
    LOG_ON("AUTH_ETH request filtered by mac");
    return;
  }
  
  MODEL.AUTH.auth_ok = true;
  // Пакет для нас. Проверим доступ
  if (!resp->access){
    LOG_ON("Access depricated");
    MODEL.AUTH.access_ok = false;
    return;
  };
  
  MODEL.AUTH.access_ok = true;
  
  for (char i = 0; i < 16 ; i++)
    MODEL.node_param[i] = resp->param[i];
  
  MODEL.node_adr = resp->ipaddr;
  setNodeCHTS();
  LOG_ON("Node auth ok! ipaddr=%d", MODEL.node_adr);
};


static bool mac_cmp(char *mac1, char *mac2){
  for (char i = 0; i < 8; i++)
    if (mac1[i] != mac2[i])
      return false;
  return true;
};

/** brief Добавляем в таблицу ответ от шлюза об авторизации узла
* Данные заносятся в таблицу и будут ретранслированы как 
* AUTH_NODE_RESP. 
*/
void set_AUTH_NODE_RESP(struct frame* frame){
  // Протокол AUTH_IP ничего не знает о структурах запросов,
  // он выполняет функцию транспортного уровня. payload
  // содержит ответ шлюза
  ASSERT(frame);
  struct AUTH_ETH_RESP *res = (struct AUTH_ETH_RESP*)frame->payload;
  
  if (frame->len != sizeof(struct AUTH_ETH_RESP)){
    LOG_ON("AUTH_IP resp from gw have wrong size= %d. struct size=%d", frame->len,
           sizeof(struct AUTH_ETH_RESP));
    return;
  };

  // Проверим что такого ответа еще нет по мак адресу
  int idx = -1;
  for (int i = 0; i < AUTH_NODE_RESP_SIZE; i++){
    // Пропускаем пустые записи
    if (!AUTH_NODE_RESP[i].RA)
      continue;
    
    // Если нашли запись с таким же мак адрессом
    if (mac_cmp(AUTH_NODE_RESP[i].res.mac, res->mac)){
      idx = i;
      break;
    };
  };

  // Не нашли запись. Ищем свободное место
  // Записи с таким же мак адресом нету. Найдем свободное место
  if (idx < 0){
    for (int i = 0; i < AUTH_NODE_RESP_SIZE; i++)
      if (!AUTH_NODE_RESP[i].RA){
        idx = i;
        break;
      };
  };
  
  if (idx < 0){
    LOG_ON("No free space in AUTH_NODE_RESP_SIZE");
    return;
  };
  
  // Добавляем запись
  MEMCPY((char*)&AUTH_NODE_RESP[idx].res, (char*)res, 
         sizeof(struct AUTH_ETH_RESP));
  AUTH_NODE_RESP[idx].RA = true;
  
  LOG_ON("AUTH RESP from GW add AUTH_NODE_RESP");
};

/** brief Подготавливает и возвращает кадр с данными запроса
*/
struct frame* get_AUTH_NODE_REQ(){
  int idx = -1;
  // Ищем первую активную запись в таблице
  for (int i = 0; i < AUTH_NODE_REQ_SIZE; i++)
    if (AUTH_NODE_REQ[i].RA){
      idx = i;
      break;
    };
  
  // Ничего не нашли
  if (idx < 0)
    return NULL;

  struct frame *fr = FR_create();
  ASSERT(fr);
  
  //Добавим данные запроса
  bool res = FR_add_header(fr, 
             (char*)(&AUTH_NODE_REQ[idx].req),
              sizeof(struct AUTH_ETH_REQ));
  ASSERT(res);
  
  // Освобождаем запись
  AUTH_NODE_REQ[idx].RA = false;
  return fr;
};

/** brief Обработка запроса авторизации от соседних узлов
* Запрос обрабатывается если данный узел авторизован  
*/
static void receiveCMD_Request(struct frame* frame){
  bool condition = MODEL.AUTH.auth_ok && MODEL.AUTH.access_ok &&
    MODEL.SYNC.synced && MODEL.NEIGH.comm_node_found;
  if (!condition){
    LOG_ON("Cant handel auth req");
    return;
  };
  
  // Сначала убедимся что этот узел уже не в списке запросов
  // проверим mac. Если узел существует то обновим запись о нем
  // вдруг что то изменилось
  struct AUTH_ETH_REQ *req = (struct AUTH_ETH_REQ*)frame->payload;
  
  int idx = -1;
  for (int i = 0; i < AUTH_NODE_REQ_SIZE; i++){
    // Пропускаем пустые записи
    if (!AUTH_NODE_REQ[i].RA)
      continue;
    
    // Если нашли запись с таким же мак адрессом
    if (mac_cmp(AUTH_NODE_REQ[i].req.mac, req->mac)){
      idx = i;
      break;
    };
  };
  
  // Записи с таким же мак адресом нету. Найдем свободное место
  if (idx < 0){
    for (int i = 0; i < AUTH_NODE_REQ_SIZE; i++)
      if (!AUTH_NODE_REQ[i].RA){
        idx = i;
        break;
      };
  };
  
  if (idx < 0){
    LOG_ON("No free space in AUTH_NODE_REQ");
    return;
  };
  
  // Добавляем запись
  AUTH_NODE_REQ[idx].RA = true;
  MEMCPY((char*)&AUTH_NODE_REQ[idx].req, (char*)req, 
         sizeof(struct AUTH_ETH_REQ));
  char *mac_ptr = AUTH_NODE_REQ[idx].req.mac;
  
  LOG_ON("AUTH REQ from mac: 0x%2x%2x%2x%2x%2x%2x%2x%2x add AUTH_NODE_REQ",
         mac_ptr[0],mac_ptr[1],mac_ptr[2],mac_ptr[4],mac_ptr[5],mac_ptr[6],
         mac_ptr[7]);
};

/** brief Передача ответа на авторизацию, полученую от шлюза
* запросившему узлу.
*/
static void AUTH_resend_RESP_from_gw(){
  static unsigned long int last = 0;
  if ((MODEL.RTC.uptime - last) < AUTH_RESP_GW_INTERVAL)
    return;
  // Обновляем время
  last = MODEL.RTC.uptime;
  
  // Ищем ответ шлюза
  int idx = -1;
  for (int i = 0; i < AUTH_NODE_RESP_SIZE; i++){
    if (AUTH_NODE_RESP[i].RA){
      idx = i;
      break;
    };
  };
  
  // Таблица пуста
  if (idx < 0) 
    return;
  
  // Нашли пакет для передачи
  // Создаем ответ
  struct frame* frame = FR_create();
  ASSERT(frame);
  
  // Добавляем в пакет данные
  bool res;
  res = FR_add_header(frame, (char*)&AUTH_NODE_RESP[idx].res,
                      sizeof(struct AUTH_ETH_RESP));
  ASSERT(res);
  
  // Добавляем в загловок тип ответа
  char cmd = AUTH_CMD_RESP;
  res = FR_add_header(frame, &cmd, sizeof(cmd));
  ASSERT(res);
  
  // Добавляем параметры передачи
  frame->meta.TS = 1;
  frame->meta.CH = MODEL.SYNC.sys_channel;
  frame->meta.PID = PID_AUTH;  
  frame->meta.NSRC = MODEL.node_adr;
  frame->meta.NDST = 0xffff;
  LOG_ON("Auth ip response from gw sended")
  RP_Send(frame);
  AUTH_NODE_RESP[idx].RA = false;;
};

void AUTH_ETH_TimeAlloc(){
    // Если узел еще не авторизован, но синхронизирован и 
  // найден ближайший сосед для передачи, то отправляем запрос
  // авторизации
  if (!MODEL.AUTH.auth_ok && MODEL.SYNC.synced &&
      MODEL.NEIGH.comm_node_found)   
        AUTH_request();
  
  // Если мы авторизированы, синхронизированы, есть сосед для
  // передачи данных и доступ нам разрешен, то будем ретранслировать
  // ответы от шлюза на запросы подключения.
  if (MODEL.AUTH.auth_ok && MODEL.SYNC.synced &&
      MODEL.NEIGH.comm_node_found && MODEL.AUTH.access_ok)    
   AUTH_resend_RESP_from_gw();
};

static void AUTH_request(){
  // Еще не наступло время передачи запроса
  if ((MODEL.RTC.uptime - last_request) < REQUEST_INTERVAL)
    return;
  // Обновляем время
  last_request = MODEL.RTC.uptime;
  
  // Создаем запрос
  struct frame* frame = FR_create();
  struct AUTH_ETH_REQ req;
  for (int i = 0; i < 8; i++)
    req.mac[i] = MODEL.node_mac[i];
  
  req.node_type = NODE_TYPE;
  req.node_ver = NODE_VER;

  bool res;
  res = FR_add_header(frame, &req, sizeof(struct AUTH_ETH_REQ));
  ASSERT(res);
  char cmd = AUTH_CMD_REQ;
  res = FR_add_header(frame, &cmd, sizeof(cmd));
  ASSERT(res);
  
  frame->meta.PID = PID_AUTH;
  LOG_ON("Node authorisation requested");
  RP_Send_COMM(frame);
};