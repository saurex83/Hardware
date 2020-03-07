#pragma once
#include "global.h"
#include "ioCC2530.h"
#include "stdio.h"

extern void DBG_CORE_HALT(void);
extern void DBG_CORE_FAULT(void);

#ifdef DEBUG
  #define LOG_ON(...) {\
    printf("%s:%d:%s -> ",__FILE__, __LINE__, __FUNCTION__); \
    printf(__VA_ARGS__); \
    printf("\r\n"); \
      }
  #define ASSERT(cond) {\
    if (!(cond)) {\
      printf("!%s:%d\r\n",__FILE__, __LINE__);\
      DBG_CORE_HALT();\
    }\
  }
  #define HALT(...) {\
      printf("%s:%d:%s -> ",__FILE__, __LINE__, __FUNCTION__);\
       printf(__VA_ARGS__); \
      DBG_CORE_HALT();\
  }
#else
  #define LOG_ON(...) {}
  #define ASSERT(cond) {if (!(cond)){DBG_CORE_FAULT();};}
  #define HALT(...) {DBG_CORE_FAULT();} 
#endif

#ifdef SIG_DEBUG
  #define PIN1 P1_0
  #define PIN2
  #define PIN3
  #define HIGH(pin) pin = 1
  #define LOW(pin) pin = 0
  #define BLINK(pin) {pin = 1; pin = 0}
  #define SIG_ON(action, pin) action(pin)
#else
  #define PIN1  
  #define PIN2
  #define PIN3
  #define HIGH(pin) {}
  #define LOW(pin) {}
  #define BLINK(pin) {}
  #define SIG_ON(action, pin) {}
#endif


#define LOG_OFF(...)
#define SIG_OFF(action, pin)

// Тяжелый ассерт в плане расхода памяти
//  #define ASSERT(cond) {\
//    if (!(cond)) {\
//      printf("%s:%d:%s -> ",__FILE__, __LINE__, __FUNCTION__);\
//      printf("\""#cond"\" Faild! \r\n");\
//      DBG_CORE_HALT();\
//    }\

