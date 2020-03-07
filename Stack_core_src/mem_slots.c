#include "debug.h"
#include "global.h"
#include "macros.h"
#include "stdbool.h"
#include "action_manager.h"
#include "mem_utils.h"  
#include "cpu.h"

//!< Максимальный размер данных в одном слоте
#define SLOT_BUFFER_SIZE 150
#define RED_ZONE_CODE 0x73

typedef char red_zone_t;

static void SW_Init(void);
module_s MS_MODULE = {ALIAS(SW_Init)};

struct property{
  char taken: 1;
};

struct slot{
  struct property property;
  red_zone_t red_zone_1;
  char buffer[SLOT_BUFFER_SIZE];
  red_zone_t red_zone_2;
};

//!< Хранилище слотов
static struct slot SLOT_POOL[SLOT_POOL_ITEMS];
//!< Количество занятых слотов
static int slot_busy;

void SW_Init(void){
  slot_busy = 0;
  for_each_type(struct slot, SLOT_POOL, slot){
    slot->property.taken = false;
    #ifdef FILL_SLOT_ZERO
      MEMSET(slot->buffer, 0, SLOT_BUFFER_SIZE);
    #endif    
    slot->red_zone_1 = RED_ZONE_CODE;
    slot->red_zone_2 = RED_ZONE_CODE;
  }
};

/**
@brief Возвращает указатель на буфер или NULL. Буфер заполнен 0
@detail 
*/
char* SL_alloc(void){  
  char *ret_ptr = NULL;
  ATOMIC_BLOCK_RESTORE{
    for_each_type(struct slot, SLOT_POOL, slot){
      if (!slot->property.taken){
        slot->property.taken = true;
        slot_busy++;
        #ifdef FILL_SLOT_ZERO
          MEMSET(slot->buffer, 0, SLOT_BUFFER_SIZE);
        #endif
        ret_ptr = slot->buffer;
        break;
      };
    };  
  };
  return ret_ptr;
};

static bool _free(char *buff){
  struct slot *slot = container_of(buff, struct slot, buffer);
  
  if (!is_array_ptr(SLOT_POOL, slot, sizeof(struct slot)))
    return false;
  
  size_t index = array_index(SLOT_POOL, slot, sizeof(struct slot));
  if (!(index < SLOT_BUFFER_SIZE))
    return false;
  
  if (slot->property.taken != true)
    return false;
    
  slot->property.taken = false;
  slot_busy--;
  return true;
}

bool SL_free(char *buff){
  bool res;
  ATOMIC_BLOCK_RESTORE{
    res = _free(buff);
  }
  return res;
};


int SL_busy(){
  ASSERT(slot_busy <= SLOT_POOL_ITEMS);
  return slot_busy;
};

int SL_available(){
  ASSERT(slot_busy <= SLOT_POOL_ITEMS);  
  return SLOT_POOL_ITEMS - slot_busy;
};

int SL_zone_check(){
  int index = 0;
  for_each_type(struct slot, SLOT_POOL, slot){
    if (!(slot->red_zone_1 == RED_ZONE_CODE &&
          slot->red_zone_2 == RED_ZONE_CODE))
      return index;
    index++;
  }
  return -1;
};

