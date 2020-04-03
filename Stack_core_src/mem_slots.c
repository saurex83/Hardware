#include "debug.h"
#include "global.h"
#include "macros.h"
#include "stdbool.h"
#include "action_manager.h"
#include "mem_utils.h"  
#include "cpu.h"
#include "mem_slots.h"

//!< Максимальный размер данных в одном слоте
#define SLOT_BUFFER_SIZE 150
#define RED_ZONE_CODE 0x73

typedef char red_zone_t;

static void SW_Init(void);
module_s MS_MODULE = {ALIAS(SW_Init)};

struct property{
  char taken: 1;
  char RX :1;
  char TX :1;
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
    slot->property.RX = false;
    slot->property.TX = false;
    #ifdef FILL_SLOT_ZERO
      MEMSET(slot->buffer, 0, SLOT_BUFFER_SIZE);
    #endif    
    slot->red_zone_1 = RED_ZONE_CODE;
    slot->red_zone_2 = RED_ZONE_CODE;
  }
};

void SW_restart(){
  SW_Init();
};

int SL_rx_slots(){
    int cnt=0;
    for_each_type(struct slot, SLOT_POOL, slot){
      if (slot->property.taken && slot->property.RX)
        cnt++;
    }
    return cnt;
}

int SL_tx_slots(){
    int cnt=0;
    for_each_type(struct slot, SLOT_POOL, slot){
      if (slot->property.taken && slot->property.TX)
        cnt++;
    }
    return cnt;
}

/**
@brief Возвращает указатель на следующий буфер tx
@param buff. buff = NULL поиск от начала списка или от последнего найденого 
 буфера.
*/
char* SL_find_tx(char* buff){  
  if (!buff){
    for_each_type(struct slot, SLOT_POOL, slot){
      if (slot->property.taken && slot->property.TX)
        return slot->buffer;
    }
    return NULL;
  }
  struct slot *geven_slot = container_of(buff, struct slot, buffer);
  geven_slot++;
  
  while (geven_slot <= &SLOT_POOL[SLOT_POOL_ITEMS]){
    if (geven_slot->property.taken && geven_slot->property.TX)
      return geven_slot->buffer;
    geven_slot++;
  }
  return NULL;
}

/**
@brief Возвращает указатель на следующий буфер rx
@param buff. buff = NULL поиск от начала списка или от последнего найденого 
 буфера.
*/
char* SL_find_rx(char* buff){  
  if (!buff){
    for_each_type(struct slot, SLOT_POOL, slot){
      if (slot->property.taken && slot->property.RX)
        return slot->buffer;
    }
    return NULL;
  }
  struct slot *geven_slot = container_of(buff, struct slot, buffer);
  geven_slot++;
  
  while (geven_slot <= &SLOT_POOL[SLOT_POOL_ITEMS]){
    if (geven_slot->property.taken && geven_slot->property.RX)
      return geven_slot->buffer;
  }
  return NULL;
}

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
        slot->property.RX = false;
        slot->property.TX = false;
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
    
  slot->property.taken = false;
  slot->property.RX = false;
  slot->property.TX = false;
  slot_busy--;
  return true;
}

bool SL_is_tx(char *buff){
  struct slot *slot = container_of(buff, struct slot, buffer);
  return slot->property.TX;
}

bool SL_is_rx(char *buff){
  struct slot *slot = container_of(buff, struct slot, buffer);
  return slot->property.RX;
}

void SL_set_tx(char *buff){
  ATOMIC_BLOCK_RESTORE{
    struct slot *slot = container_of(buff, struct slot, buffer);
    slot->property.TX = true;
  }
}

void SL_set_rx(char *buff){
  ATOMIC_BLOCK_RESTORE{
    struct slot *slot = container_of(buff, struct slot, buffer);
    slot->property.RX = true;
  }
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

