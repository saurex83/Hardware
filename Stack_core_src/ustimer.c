#include "ioCC2530.h"
#include "ustimer.h"
#include "stdint.h"
#include "action_manager.h"
#include "stdbool.h"


#define ACCURATE_VAL_US 500 

// Максимальное кол-во тактов таймера. Константа 40 бит, 5 байт
static const uint32_t MAC_TIMER_MAX = 0xFFFFFFFFUL; 
static uint32_t DELAY_CALIBRATE_TICS; // Калибровка функции задержки

static void DelayCalibrate(void);
static void HW_Init(void);  
module_s UST_MODULE = {ALIAS(HW_Init)};


void HW_Init(void){
  // Запускаем таймер, LATCH MODE = 1
  // Latch mode фиксирует значение таймера переполнения при чтении T2M0
  // ЗАпускаем MAC таймер без синхронизации с кварцем 32.768к
  T2CTRL = (1<<0) | (1<<3); 
  // Калибровка функции задержки
  DelayCalibrate();
}

ustime_t UST_now(void){
  ustime_t now;
  ((char*)&now)[0] = T2M0;
  ((char*)&now)[1] = T2M1;
  ((char*)&now)[2] = T2MOVF0;
  ((char*)&now)[3] = T2MOVF1;
  return now;
}

bool UST_time_over(stamp_t beg, ustime_t wait){
  stamp_t now = UST_now(); 
  ustime_t delta = UST_interval(beg, now);
  return (delta > wait) ?  true:false;
}

ustime_t UST_interval(stamp_t beg, stamp_t end){
  return (end >= beg) ? 
    TICKS_TO_US(end - beg) :
    TICKS_TO_US(MAC_TIMER_MAX - beg + end);
};

void UST_delay(ustime_t time){
  stamp_t now = UST_now();
  stamp_t len = US_TO_TICKS(time);
  stamp_t offset = (time > DELAY_CALIBRATE_TICS) ?
                    DELAY_CALIBRATE_TICS : 0;
  while (UST_now() < (now + len - offset));
}


stamp_t _interval(stamp_t beg, stamp_t end){
  return (end >= beg) ? 
    (end - beg) :
    (MAC_TIMER_MAX - beg + end);
};

/**
@brief Калибровка функции задержки TIM_delay
*/
static void DelayCalibrate(void){
  DELAY_CALIBRATE_TICS = 0;  
  stamp_t start = UST_now();
  UST_delay(ACCURATE_VAL_US);
  stamp_t end = UST_now();
  DELAY_CALIBRATE_TICS = _interval(start, end) -
                          US_TO_TICKS(ACCURATE_VAL_US);
};
