#include "mem_utils.h"
#include "debug.h"

/**
@file Реентерабельное копирование
*/

void MEM_memcpy(struct memcpy *memcpy){
  while (memcpy->len > 0){
    *memcpy->dst = *memcpy->src;
    memcpy->dst++;
    memcpy->src++;
    memcpy->len--;
  }
};

void MEM_memset(struct memset *memset){
  for (char i = 0; i < memset->len; i++)
    memset->dst[i] = memset->val;
};

//void MEM_memset(struct memset *memset){
//  while (memset->len > 0){
//    *memset->dst = memset->val;
//    memset->dst++;
//    memset->len--;
//  }  
//};


