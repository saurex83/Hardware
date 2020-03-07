#include "utest.h"
#include "stddef.h"
#include "stdbool.h"

static int test1(void);
static int test2(void);
static int test3(void);
struct test_unit UT_FR_1 = {.name = "FR:create-delete", .fun = test1};
struct test_unit UT_FR_2 = {.name = "FR:create-full", .fun = test2};
struct test_unit UT_FR_3 = {.name = "FR:header manipulation", .fun = test3};
#include "frame.h"

//struct frame* FR_create();
//void FR_delete(struct frame*);
//void FR_add_header(struct frame*, void *head, char len);
//void FR_del_header(struct frame*, char len);
//int FR_busy();
//int FR_available();

static bool ch_cmp(void *dst, void *src, size_t len){
  while (len-- > 0){
    if ( *(char*)dst != *(char*)src)
      return false;
  }
  return true;
}

static bool ch_cmp_val(void *dst, char val, size_t len){
  while (len-- > 0){
    if ( *(char*)dst != val)
      return false;
  }
  return true;
}

static int test3(void){
  int test_res = 0;
  struct frame *fr = FR_create();
  char hdr[10] = {1,2,3,4,5,6,7,8,9,10};
  FR_add_header(fr, hdr, sizeof(hdr));
  if (!ch_cmp(hdr, fr->payload, sizeof(hdr)))
      test_res = 1;
  
  if (fr->len != sizeof(hdr))
      test_res = 2;
  
  FR_del_header(fr, 5);
  if (!ch_cmp(&hdr[5], fr->payload, sizeof(hdr) - 5))
      test_res = 3;  
  
  if (fr->len != sizeof(hdr) - 5)
    test_res = 4;
  
  if (!ch_cmp_val(&fr->payload[5], 0, MAX_PAYLOAD_SIZE - 5))
    test_res = 5;  
  
  FR_del_header(fr, 5);
  if (!ch_cmp_val(&fr->payload, 0, MAX_PAYLOAD_SIZE))
    test_res = 6; 
  
  if (fr->len != 0)
      test_res = 7;
  
  if (FR_add_header(fr, hdr, 200))
    test_res = 8;
 
  if (FR_del_header(fr, 200))
    test_res = 9;
  return test_res;
}

static int test2(void){
  int test_res = 0;
  struct frame* all[20];
  
  for (int i = 0; i < 20; i++)
    all[i] = FR_create();

// Нету NULL  
  for(int i = 0; i < 20; i++)
    if (all[i] == NULL)
      test_res = 1;

// Все слоты должны быть разные 
  for(int i = 1; i < 20; i++)
    if (all[i] == all[i - 1])
      test_res = 2;   
  
  if (FR_busy() != 20)
    test_res = 3;
  
  if (FR_available() != 0)
    test_res = 4;
 
  // Выделяемм еще один. он должен быть NULL
  struct frame* fr = FR_create();
  if (fr)
    test_res = 5;
  
   if (FR_busy() != 20)
    test_res = 6;
  
  if (FR_available() != 0)
    test_res = 7; 
  
// Удаляем все что создали и контролим результат
  for (int i = 0; i < 20; i++)
    if (!FR_delete(all[i]))
      test_res = 8;   

   if (FR_busy() != 0)
    test_res = 9;
  
  if (FR_available() != 20)
    test_res = 10;  
  
// Еще разок пробуем все удалить 
  for (int i = 0; i < 20; i++)
    if (FR_delete(all[i]))
      test_res = 11; 
  
   if (FR_busy() != 0)
    test_res = 12;
  
  if (FR_available() != 20)
    test_res = 13;
  
  return test_res;
}

static int test1(void){
  int test_res = 0;
  
  if (FR_busy() != 0)
    test_res = 1;
  
  if (FR_available() != 20)
    test_res = 2;
  
  struct frame *fr = FR_create();
  
  if (!fr)
    test_res = 3;
  
  if (FR_busy() != 1)
    test_res = 4;
  
  if (FR_available() != 19)
    test_res = 5;  
  
  if (!FR_delete(fr))
    test_res = 6;
  
  if (FR_busy() != 0)
    test_res = 7;
  
  if (FR_available() != 20)
    test_res = 8;   
  
  return test_res;
}