#pragma once


#define ALIAS(name) .name = name

/**
@brief Главная структура модуля
*/
typedef struct {
  void (*HW_Init)(void);  
  void (*SW_Init)(void); 
  void (*IRQ_Init)(void); 
  void (*Cold_Start)(void); 
  void (*Hot_Start)(void); 
} module_s;

/**
@brief Представление модели модуля
*/
#define AM_MODEL AM
struct AM{
  int a;
};

void AM_Callback(void);
void AM_Cold_start(void);
void AM_Hot_start(void);
void AM_IRQ_Init(void);
void AM_HW_Init(void);
void AM_SW_Init(void);
void AM_set_callback(void (*fn)(void));
void AM_Callback(void);