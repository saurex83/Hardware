#include "model.h"
#include "debug.h"
#include "action_manager.h"
#include "ethernet.h"
#include "auth_ip.h"
#include "auth_eth.h"
#include "neigh.h"
#include "route.h"

static void HP_callback(void);
static void stack_init();

static void (*userTimeAllocation)(void);

//HP - high protocol

void HP_Init(void){
  userTimeAllocation = NULL;
  AM_HW_Init();
  AM_SW_Init();
  LOG_ON("Hardware inited");
  MODEL.SYNC.mode = 1;
  MODEL.TM.MODE = 1;
  AM_set_callback(HP_callback);
  stack_init();
}

void setUserTimeAllocation(void (*fn)(void)){
  userTimeAllocation = fn;
};

bool NeocoreReady(){
  bool flt = MODEL.AUTH.auth_ok && MODEL.AUTH.access_ok && 
             MODEL.SYNC.synced && MODEL.NEIGH.comm_node_found;
  return flt;
};


static void stack_init(){
  AUTH_IP_Init();
  AUTH_ETH_Init();
  NP_Init();
  RP_Init();
  LOG_ON("Neocore stack inited");
};

void HP_ReInit(){
  stack_init();
};

static void HP_callback(void){
  // Обрабатываем стек
  ethernet_process();
  
  // Выделяем пользователю время
  if (userTimeAllocation)
    userTimeAllocation();
};

