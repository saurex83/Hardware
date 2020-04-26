#include "udp.h"
#include "debug.h"
#include "frame.h"
#include "protocol_defs.h"
#include "ip.h"

struct udp_bind{
  bool RA;
  char port;
  void (*fn)(struct frame *frame);
};

// Таблица для вызова обработчиков
#define BIND_SIZE 5
static struct udp_bind BIND[BIND_SIZE];


void UDP_Send(char port, char *ptr, char size){
  if (size > 64){
    LOG_ON("UDP data too big");
    return;
  }
  
  struct frame *frame = FR_create(); 
  ASSERT(frame);
  bool res = FR_add_header(frame, ptr, size);
  ASSERT(res);
  
  res = FR_add_header(frame, &port, sizeof(char));
  ASSERT(res);
  
  frame->meta.IPP = IPP_UDP;
  IP_Send(frame);
  
  LOG_ON("UDP frame send to gw. port=%d", port);
};

bool UDP_bind_port(char port, void (*fn)(struct frame *frame)){
  for (int i = 0 ; i < BIND_SIZE; i++)
    if (!BIND[i].RA){
      BIND[i].port = port;
      BIND[i].fn = fn;
      return true;
    };
  return false;
};

void UDP_Recive(struct frame *frame){
  if (frame->len == 0){
    LOG_ON("UDP wrong size");
  };
  
  char port = frame->payload[0];
  bool res = FR_del_header(frame, 1);
  ASSERT(res);
  
  for (int i = 0; i < BIND_SIZE; i++)
    if (BIND[i].port == port && BIND[i].RA){
        BIND[i].fn(frame);
        return;
    };
};