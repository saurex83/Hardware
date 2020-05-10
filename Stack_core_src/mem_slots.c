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

#ifndef SLOT_POOL_ITEMS
#define SLOT_POOL_ITEMS 20
#endif

#ifndef SLOT_TX_ITEMS   
#define SLOT_TX_ITEMS 7
#endif

#ifndef SLOT_RX_ITEMS   
#define SLOT_RX_ITEMS 7
#endif

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
//!< Количество пакетов помеченных на прием
static int slot_rx_cnt;
//!< Количество пакетов помеченных на передачу
static int slot_tx_cnt;

void SW_Init(void){
  slot_busy = 0;
  slot_rx_cnt = 0;
  slot_tx_cnt = 0;
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
    geven_slot++;
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

/**
@brief Копируем слот. 
@details При копировании сбрасываем принадлежность к буферам RX/TX
@ return указатель на новый слот или NULL
*/
char* SL_copy(char *buff){
  char *ret_ptr = SL_alloc();
  if (!ret_ptr)
    return NULL;
  
  ATOMIC_BLOCK_RESTORE{
    struct slot *src = container_of(buff, struct slot, buffer);
    struct slot *dst = container_of(ret_ptr, struct slot, buffer);
    MEMCPY((char*)dst, (char*)src, sizeof(struct slot));
    dst->property.RX = false;
    dst->property.TX = false;
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
    slot_tx_cnt++;
    struct slot *slot = container_of(buff, struct slot, buffer);
    slot->property.TX = true;
  }
}

void SL_set_rx(char *buff){
  ATOMIC_BLOCK_RESTORE{
    slot_rx_cnt++;
    struct slot *slot = container_of(buff, struct slot, buffer);
    slot->property.RX = true;
  }
}


bool SL_free(char *buff){
  bool res;
  ATOMIC_BLOCK_RESTORE{
    if (SL_is_tx(buff))
      slot_tx_cnt--;
    else if (SL_is_rx(buff))
      slot_rx_cnt--;
    
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

int SL_tx_available(){
  ASSERT(slot_tx_cnt <= SLOT_TX_ITEMS);  
  return SLOT_TX_ITEMS - slot_tx_cnt;
};

int SL_rx_available(){
  ASSERT(slot_rx_cnt <= SLOT_RX_ITEMS);  
  return SLOT_RX_ITEMS - slot_rx_cnt;
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

