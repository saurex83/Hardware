#include "model.h"
#include "debug.h"
#include "frame.h"
#include "ethernet.h"
#include "protocol_defs.h"

#define REQUEST_INTERVAL 30
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

void AUTH_ETH_Receive(struct frame *frame){
  LOG_ON("AUTH_ETH receive frame");
  // Проверим пакет.
  // Мы можем получить пакет REQ или RESP
  // Нужно типы пакетов разделить по нормальному а не cmd
  // Для этого первым байтом делаем тип команды/данных а потом
  // отрезаем от фрейма
  
  bool filter_rule = (frame->meta.TS == 1) &&
                     (frame->meta.NDST == 0xffff) &&
                     (frame->len > 0);  
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
  LOG_ON("Node auth ok! ipaddr=%d", MODEL.node_adr);
};

/** brief Обработка запроса авторизации
* Запрос обрабатывается если данный узел авторизован  
*/
static void receiveCMD_Request(struct frame* frame){
  if (MODEL.AUTH.auth_ok == false)
    return;
  
  // TODO Сохранить запрос в таблице и добавить геттер запросов.
  // Авторизация уровня IP будет опрашивать таблицу и
  // общаться со шлюзом по этому вопросу.
  LOG_ON("AUTH REQ received and droped");
};

void AUTH_ETH_TimeAlloc(){
   // Выходим если узел авторизован
  if (MODEL.AUTH.auth_ok == true)
     return;
  
   AUTH_request(); 
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
  
  frame->meta.TS = 1;
  frame->meta.CH = MODEL.SYNC.sys_channel;
  frame->meta.PID = PID_AUTH;
  frame->meta.NDST = 0xffff;
  frame->meta.NSRC = 0;
 
  eth_send(frame); 
  LOG_ON("AUTH req add to tx");
};