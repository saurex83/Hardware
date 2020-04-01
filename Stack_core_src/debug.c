#include "debug.h"
#include "action_manager.h"

/**
@file
*/

static void HW_Init(void);  
static void uart_init(void);
 
module_s DBG_MODULE = {ALIAS(HW_Init)};

static void HW_Init(void){
#ifndef DEBUG
  return;
#endif
#ifdef PRINT_TO_TERMINAL  
  uart_init();
#endif
#ifdef SIG_DEBUG
  P1DIR |=0x01;
  P1_0 |= 0x01;  
#endif
};  


void DBG_CORE_HALT(void){
  while(1);
}

void DBG_CORE_FAULT(void){
  while(1);
}

// Настройка порта uart для отладки
static void uart_init(void){
  // Выбор расположения UART на выводах
  char U0CFG = 0; // 0={TX=P0_3, RX=P0_2
  PERCFG |= (U0CFG<<0); 
  // U0UCR регистр настройки режима uart. меня устраивает по умолчанию
  U0CSR = (1<<7); // Выбираем режим uart  
  // Настройка скорости передачи данных на 2М  
  U0BAUD = 0;  // табличные значения из pdf
  U0GCR =  16;
  // Включаем альтернативные функции выводов
  P0SEL = (1<<2)|(1<<3);
  U0CSR |= (1<<6);
}

//#ifdef PRINT_TO_TERMINAL 
//#include <yfuns.h>
//_STD_BEGIN
//#pragma module_name = "?__write"
//
__near_func int putchar(int x){
  while( U0CSR&(1<<0));
  U0DBUF = x;
  return x;
}

