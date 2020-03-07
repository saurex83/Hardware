#include "debug.h"
#include "alarm_timer.h"
#include "cpu.h"

// тестирование вставками. Теперь переопределить функции нельзя
// void TM_IRQ(nwtime_t time) теперь вручную мониторить
void u_alarm_timer(void){
  nwtime_t alarm = 500;
  INTERRUPT_ENABLE();
  AT_set_alarm(alarm);
  while(1);
}