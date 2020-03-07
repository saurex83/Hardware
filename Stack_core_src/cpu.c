#include "action_manager.h"
#include "ioCC2530.h"

/**
@file
*/

static void HW_Init(void);   
 
module_s CPU_MODULE = {ALIAS(HW_Init)};

static void HW_Init(void){
  /* Сбрасываем бит OSC32K, будет выбран 32.768 кварц */
  CLKCONCMD &= ~(1<<7); 
  /* Ждем пока бит не станет 0 */
  while (CLKCONSTA & (1<<7)); 
  /* Переходим на 32 Мгц */
  CLKCONCMD = 0;// (1<<3);
  while (CLKCONSTA&(1<<6));
  EA = 1;
};  
