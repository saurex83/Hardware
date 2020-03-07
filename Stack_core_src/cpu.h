#include "ioCC2530.h"

#define INTERRUPT_ENABLE() EA = 1
#define INTERRUPT_DISABLE() EA = 0

#define ATOMIC_START { char _save_EA = EA; EA = 0; do {
#define ATOMIC_END } while(0); EA = _save_EA;};

static inline char __cli(void){
  EA = 0;
  return 1;
}

#define ATOMIC_BLOCK_RESTORE for (              \
  char _save_EA = EA, __ToDo = __cli();         \
  __ToDo ;                                      \
  __ToDo = 0, EA = _save_EA )

#define ATOMIC_BLOCK_FORCEON  for (             \
  char __ToDo = __cli();                        \
  __ToDo ;                                      \
  __ToDo = 0, EA = 1 )