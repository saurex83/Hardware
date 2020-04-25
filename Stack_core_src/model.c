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
  
  MODEL.node_adr = 0;
  MODEL.node_mac[0] = 0x11;
  MODEL.node_mac[1] = 0x22;
  MODEL.node_mac[2] = 0x33;
  MODEL.node_mac[3] = 0x44;
  MODEL.node_mac[4] = 0x55;
  MODEL.node_mac[5] = 0xaa;
  MODEL.node_mac[6] = 0xee;
  MODEL.node_mac[7] = 0xff;
  
  MODEL.AUTH.auth_ok = false;
  MODEL.AUTH.access_ok = false;
  
  MODEL.NEIGH.comm_node_found = false;
}; 


