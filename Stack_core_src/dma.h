#include "stdint.h"

/**
@file Выделяет память для структур DMA.
@detail Структуры DMA с 1-4 идут последовательно
*/

#define ST_DEF(STRUCT, FILD, VAL)  STRUCT.FILD = VAL
#define HADDR(ADDR) ((uint16_t)ADDR >> 8)
#define LADDR(ADDR) ((uint16_t)ADDR)

typedef struct //!< Структура с настройками DMA 
{
  uint8_t  SRCADDRH;
  uint8_t  SRCADDRL;
  uint8_t  DSTADDRH;
  uint8_t  DSTADDRL;
  struct  {
  uint8_t   LENH        :5;
  uint8_t   VLEN        :3;
  };
  struct  {
  uint8_t   LENL        ;
  };
  struct  {
  uint8_t   TRIG        :5;
  uint8_t   TMODE       :2;
  uint8_t   WORDSIZE    :1;
  };
  struct {
  uint8_t   PRIORITY    :2;
  uint8_t   M8          :1;
  uint8_t   IRQMASK     :1;
  uint8_t   DESTINC     :2;
  uint8_t   SRCINC      :2;
  };
} __attribute__((packed)) DMA_s ;

extern DMA_s DMA_CH[];
