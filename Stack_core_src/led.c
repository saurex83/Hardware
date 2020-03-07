#include "action_manager.h"

static void HW_Init(void);
static void SW_Init(void);
static void Cold_Start(void);
static void Hot_Start(void);

module_s LED_MODULE = {ALIAS(SW_Init), ALIAS(Cold_Start), 
  ALIAS(Hot_Start), ALIAS(HW_Init)};

static void HW_Init(void){};
static void SW_Init(void){};
static void Cold_Start(void){};
static void Hot_Start(void){};