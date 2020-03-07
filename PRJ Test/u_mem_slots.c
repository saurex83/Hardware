#include "utest.h"
#include "stddef.h"

static int test1(void);
static int test2(void);
static int test3(void);

struct test_unit UT_MS_1 = {.name = "MS:create-delete", .fun = test1};
struct test_unit UT_MS_2 = {.name = "MS:create-full", .fun = test2};
struct test_unit UT_MS_3 = {.name = "MS:zone-check", .fun = test3};

#include "mem_slots.h"

//int SL_zone_check();
//int SL_available();
//int SL_busy();
//bool SL_free(char *buff);
//char* SL_alloc(void);
static int test3(void){
  int test_res = 0;
  
  if (!SL_zone_check())
    test_res = 1;
  
  // Нарушаем границу во втором слоте
  char *sl1 = SL_alloc();
  char *sl2 = SL_alloc();
  char *sl3 = SL_alloc();
  for (int i = 0; i < 180; i++)
    sl3[i] = 0x55;
  
  if (SL_zone_check()!=2)
    test_res = 2;  
  
  return test_res;
}

static int test2(void){
// Используем полностью все фреймы
  char *all[20];
  int test_res = 0;
  
  for (int i = 0; i < 20; i++)
    all[i] = SL_alloc();

// Нету NULL  
  for(int i = 0; i < 20; i++)
    if (all[i] == NULL)
      test_res = 1;

// Все слоты должны быть разные 
  for(int i = 1; i < 20; i++)
    if (all[i] == all[i - 1])
      test_res = 2;  

// Все слоты занятыы
  if (SL_busy() != 20)
    test_res = 3;  

// Свободных слотов нет
  if (SL_available() != 0)
    test_res = 4;   
  
// Слотов нет и при выделении должен быть NULL  
  char *sl =  SL_alloc();
  if (sl)
    return 5;  
  
// Убедимся что счетчики не уехали
// Все слоты занятыы
  if (SL_busy() != 20)
    test_res = 6;  

// Свободных слотов нет
  if (SL_available() != 0)
    test_res = 7; 
  
// Удаляем все что создали и контролим результат
  for (int i = 0; i < 20; i++)
    if (!SL_free(all[i]))
      test_res = 8; 

// Все слоты свободны
  if (SL_busy() != 0)
    test_res = 9;  

  if (SL_available() != 20)
    test_res = 10;
  
// Пробуем удалить уже удаленное рез. должен быть false  
  for (int i = 0; i < 20; i++)
    if (SL_free(all[i]))
      test_res = 11;
  
// Все слоты свободны
  if (SL_busy() != 0)
    test_res = 12;  

  if (SL_available() != 20)
    test_res = 13;
  
  return test_res;
}

static int test1(void){
  // Простой тест на создание и удаление
  bool test_res = 0;
  
  if (SL_busy() != 0)
    test_res = 1;  

  if (SL_available() != 20)
    test_res = 2;    
  
  char* slot = SL_alloc();
  if (SL_busy() != 1)
    test_res = 3;
  
  if (SL_available() != 19)
    test_res = 4;  
  
  SL_free(slot);
  if (SL_busy() != 0)
    test_res = 5;  
  
  if (SL_available() != 20)
    test_res = 6; 
  
  return test_res;
}

