#include "model.h"
#include "action_manager.h"

/**
@file
*/

MODEL_s MODEL;

  
static void SW_Init(void); 

module_s MD_MODULE = {ALIAS(SW_Init)};

 
static void SW_Init(void){
  for (int i = 0; i < sizeof(MODEL.PWR_SCAN.energy); i++)
    MODEL.PWR_SCAN.energy[i] = -127;
}; 


