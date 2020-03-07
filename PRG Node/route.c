#include "model.h"
#include "debug.h"
#include "buffer.h"
#include "protocol_defs.h"
#include "ip.h"
#include "neigh.h"
#include "balancer.h"

#define ROUTE_TABLE_ITEMS 20

struct route_record{
  unsigned int nsrc;
  unsigned int fsrc;
  unsigned long update_time;
  bool record_active;
} __attribute__((packed));

static struct route_record ROUTE_TABLE[ROUTE_TABLE_ITEMS];

void RP_Receive(struct frame *frame){
  #warning update route tables
  switch (frame->meta.PID){
  case PID_IP: 
    IP_Receive(frame);
    break;
  case PID_NP: 
    NP_Receive(frame);
    break;
  default:
    return; // Пакет удалит ethernet
  };
  
  TB_Receive(frame);  
};

void RP_Send_GW(struct frame *frame){
};

void RP_Send(struct frame *frame){
};

void RP_SendRT_GW(struct frame *frame){
};

void RP_SendRT_RT(struct frame *frame){
};