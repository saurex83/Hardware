#include "alarm_timer.h"
#include "time_manager.h"
#include "action_manager.h"
#include "ioCC2530.h"
#include "stdbool.h"
#include "stdint.h"
#include "debug.h"
#include "cpu.h"

/**
@file
*/

#define MAX_NWTIME (nwtime_t)32767

static void HW_Init(void);  
static inline void IRQEnable(bool state);
static uint32_t ReadTimer(void);
static inline bool isIRQEnable(void);
static inline uint32_t calcCompareTime(nwtime_t time);
static void loadTimerCompare(uint32_t ticks);

module_s AT_MODULE = {ALIAS(HW_Init)};
static nwtime_t TOFFSET; 
static uint32_t COMPARE_TIME; //!< Значение в регистре compare


static void HW_Init(void){
  TOFFSET = 0;
  COMPARE_TIME = 0;
  IRQEnable(false);
};

static void SW_Init(void){
  TOFFSET = 0;
  COMPARE_TIME = 0;
  IRQEnable(false);
}; 

void AT_enable(bool state){
  ATOMIC_BLOCK_RESTORE{
    IRQEnable(state);
  }
};

nwtime_t AT_interval(nwtime_t beg, nwtime_t end){
  return (beg > end) ? (beg - end) : (MAX_NWTIME - beg + end);
}

void AT_set_time(nwtime_t time){
  ASSERT(time <= MAX_NWTIME);
  nwtime_t timer = ReadTimer();
  TOFFSET = time - timer;
  TOFFSET &= 0x7FFF;
  // После установки времени нужно изменить compare time в таймере
  // Но только в случаи если прерывание активно
  if (isIRQEnable())
    AT_set_alarm(COMPARE_TIME);
}

nwtime_t AT_time(void){
  uint32_t val = ReadTimer();
  // TOFFSET = NETWORK TIME - TIMER
  // NETWORK TIME = TIMER + TOFFSET
  val +=TOFFSET;
  val &= 0x7FFF; // Приводим значения таймера к 0-32767
  return val;
}

/**
@brief Ждем наступления определенного момента времени
*/
void AT_wait(nwtime_t time){
  ASSERT(time <= MAX_NWTIME);  
  static union 
  {
    uint32_t val;
    uint8_t fild[4];
  } val;
  val.val = 0;
  // TOFFSET = NETWORK TIME - TIMER
  // NETWORK TIME = TIMER + TOFFSET
  // Ждем синхронизацию таймера после пробуждения
  while (!(SLEEPSTA & 0x01));
  do{
    // Соблюдаем порядок чтения регисторов ST  
    val.fild[0] = ST0;
    val.fild[1] = ST1;
    val.fild[2] = ST2;
    val.val +=TOFFSET;
    val.val &= 0x7FFF; // Приводим значения таймера к 0-32767    
  } while (val.val < time);
}

/**
@brief Разрешение прерываний таймера сна
@params[in] state = true - разрешить обработку прерываний
*/
static inline void IRQEnable(bool state){
  if (state)
    STIE = 1;
  else
    STIE = 0;
}

/**
@brief Устанавливает время прерывания
@details Время прерывания устанавливается относительно времени сети
,к примеру, что бы проснуться в таймслот номер 0, нужно установить ticks = 0.
Процедура учитывает текущее значение таймера и перерасчитывает значение ticks. 
@params[in] ticks время сети в тиках когда нужно проснуться
*/
void AT_set_alarm(nwtime_t alarm){
  ASSERT(alarm <= MAX_NWTIME);
  COMPARE_TIME = alarm; // Сохраняем установленное значение
  uint32_t compare_time = calcCompareTime(alarm);
  loadTimerCompare(compare_time);
  IRQEnable(true);
}

/**
@brief Вычисляет значение, которое нужно установить в регистр compare таймера.
@params[in] time время сети в тикак
*/
static inline uint32_t calcCompareTime(nwtime_t time){   
    uint32_t timer = ReadTimer(); // Текущее значение счетчика
    //NETWORK TIME = TIMER + TOFFSET  
    // Приводим такты к тактам таймера
    uint16_t ticks_offset = (time - TOFFSET) & 0x7FFF;
    uint32_t cmp_time = timer & ~0x7FFF; // Убираем младшие 15 бит
    cmp_time |= ticks_offset; // Вычисляем новое время
    if (cmp_time <= timer){
      cmp_time += 0x8000;
      cmp_time &=0xFFFFFF;
    }
     LOG_OFF("Timer = %lu, Ticks = %d,CMP = %lu \r\n",
          timer, ticks, cmp_time );
    return cmp_time;
}

/**
@brief Возвращает текущее значение таймера
@return Текущие ticks
*/
static uint32_t ReadTimer(void){
  static union 
  {
    uint32_t val;
    uint8_t fild[4];
  } ret_val;
  ret_val.val = 0;
  // Ждем синхронизацию таймера после пробуждения
  while (!(SLEEPSTA & 0x01));
  // Соблюдаем порядок чтения регисторов ST  
  ret_val.fild[0] = ST0;
  ret_val.fild[1] = ST1;
  ret_val.fild[2] = ST2;
  return ret_val.val;
}

/**
@brief Устанавливает время пробуждения микроконтролера
@params[in] ticks 24 битное значение времени пробуждения в ticks
*/
static void loadTimerCompare(uint32_t ticks){
  union 
  {
    uint32_t val;
    uint8_t fild[4];
  } value;
  value.val = ticks;
  // Ждем разрешения на запись нового значения
  while (!(STLOAD & 0x01));
  // Регистр ST0 должен быть записан в последнию очередь
  ST2 = value.fild[2];
  ST1 = value.fild[1];
  ST0 = value.fild[0];
}

/**
@brief Проверка активности прерывания таймера
@return true если прерывание установленно
*/
static inline bool isIRQEnable(void){
  return STIE;
}

/**
@brief Обработчик прерывания таймера сна
*/
#pragma vector=ST_VECTOR
__interrupt void TimerCompareInterrupt(void){ 
  nwtime_t time = AT_time();
  // Отключаем прерывание таймера. Забота пользователя его включить
  IRQEnable(false); 
  TM_IRQ(time); // Передаем управление менеджеру времени
  STIF = 0; // Очищаем флаг прерывания
}