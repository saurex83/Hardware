#include "debug.h"
#include "mem_utils.h"

void u_mem_utils(void){
  char tst1[10] = {1,2,3,4,5,6,7,8,9,10};
  char tst2[10];
  int cmp_res;
  
  struct memcpy memcpy = {.src = tst1, .dst = tst2, .len = sizeof(tst1)};
  MEM_memcpy(&memcpy);
  
  for (int i = 0; i < sizeof(tst1); i++){
    cmp_res = tst1[i] == tst2[i];
    ASSERT(cmp_res == 0);
    }
  LOG_ON("MEM_memcpy ok!");
  
  struct memset memset = {.dst = tst1, .val = 7, .len = sizeof(tst1)};
  MEM_memset(&memset);

  for (int i = 0; i < sizeof(tst1); i++){
    cmp_res = tst1[i] == 7;
    ASSERT(cmp_res == 0);
    }
  LOG_ON("MEM_memset ok!");
}