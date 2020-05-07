#include "controller.h"

static void send_ip_frame(){
  bool flt = MODEL.AUTH.auth_ok && MODEL.AUTH.access_ok;
  if (!flt)
    return;
  
 static unsigned long last_send = 0;
 unsigned long now = MODEL.RTC.uptime;
 if ((now - last_send) < 30)
   return;
 last_send = now;
 
 char data[] = {1,2,3,4,5};
 UDP_Send(99, data, sizeof(data));
 LOG_ON("UDP SENDED1");
};


static void user_time_alloc(void){
  // measure_process()
  // Запуск измерений. им не нужена авторизация
  
  // Следующим задачам нужна авторизация и возможность передачи
  if (!NeocoreReady())
    return;
  // statisticAgregate()
  // measureAgregate()
  // nodeHeathControl() Контролируем стек и параметры.
  //
  send_ip_frame();
};

#define ONE_SEC 1000000U

void main(void){
  HP_Init();
  setUserTimeAllocation(user_time_alloc);
  
  while (true){
    while (!network_sync(ONE_SEC));
    LOG_ON("Network synced");
    Neocore_start();
    LOG_ON("Network lost sync. Resync");
    AM_SW_Init();
    HP_Init();
  }
};
