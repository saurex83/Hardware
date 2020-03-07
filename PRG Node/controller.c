#include "model.h"
#include "debug.h"
#include "buffer.h"
#include "action_manager.h"
#include "ethernet.h"

static void HP_callback(void);

void HP_Init(void){
    AM_set_callback(HP_callback);
#warning Call other protocols init
}

static void HP_callback(void){
  ethernet_process();
};

