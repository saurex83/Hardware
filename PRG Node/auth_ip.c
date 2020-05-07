#include "tcp.h"
#include "debug.h"
#include "model.h"
#include "auth_eth.h"
#include "frame.h"
#include "ip.h"
#include "protocol_defs.h"

// Инетрвал отправки запросов авторизации шлюзу в секундах
#define PERIOD_SEND_IP_REQ 5
enum AUTH_IP_TYPES {AUTH_CMD_REQ = 0, AUTH_CMD_RESP = 1};

void AUTH_IP_Init(){
    // Возможно инициализировать нужно таймеры
};

void AUTH_IP_Recive(struct frame *frame){
  // Проверим размер
  if (frame->len == 0){
    LOG_ON("Wrong size");
    return;
  };
  
  // Отрежим команду
  bool res = FR_del_header(frame,1);
  
  // Передадим ответ нижнему уровню. пусть разбирается что к чему
  set_AUTH_NODE_RESP(frame);
  LOG_ON("AUTH_IP gw resp add to auth_eth");
};

void AUTH_IP_TimeAlloc(){
  // Для работы протокола требуется что бы мы были зарегестрированы в 
  // сети
  bool flt = MODEL.AUTH.auth_ok && MODEL.SYNC.synced &&
      MODEL.NEIGH.comm_node_found && MODEL.AUTH.access_ok;
  
  if (!flt)
    return;
  
  // Мы должны из протокола AUTH_ETH выбирать принятые запросы
  // и переправлять ответы шлюзу. Запросы отправляю с неким периодом
  
  static unsigned long time = 0;
  unsigned long now = MODEL.RTC.uptime;
  if (now - time < PERIOD_SEND_IP_REQ)
    return;
  time = now;
  
  struct frame* fr = get_AUTH_NODE_REQ();
  // Нечего отправлять
  if (!fr)
    return;
  
  // Добавим тип запроса
  char cmd = AUTH_CMD_REQ;
  bool res = FR_add_header(fr, &cmd, sizeof (char));
  ASSERT(res);
  
  fr->meta.IPP = IPP_AUTH;
  IP_Send(fr);
  LOG_ON("AUTH_IP req send to GW");
};