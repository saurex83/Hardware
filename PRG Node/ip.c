#include "model.h"
#include "debug.h"
#include "route.h"
#include "udp.h"
#include "tcp.h"
#include "auth_ip.h"
#include "protocol_defs.h"


struct IP_H{
  char ETX;
  unsigned int FDST;
  unsigned int FSRC;
  char IPP;
} __attribute__((packed));


static bool frame_filter(struct frame *fr){  
  if (fr->len < sizeof(struct IP_H)){
    LOG_ON("Filtered by size");
    return false;
  };
  return true;
};

static void fill_meta(struct frame *fr){
  struct IP_H *iph = (struct IP_H*)fr->payload;
  fr->meta.ETX = iph->ETX;
  fr->meta.FDST = iph->FDST;
  fr->meta.FSRC = iph->FSRC;
  fr->meta.IPP = iph->IPP;
};

static void IPP_process(struct frame *fr){
  switch(fr->meta.IPP){
    case IPP_UDP:
      LOG_ON("IP is UDP");
      UDP_Recive(fr);
      break;
    case IPP_TCP:
      LOG_ON("IP is TCP");
      TCP_Recive(fr);
      break;    
    case IPP_AUTH:
      LOG_ON("IP is AUTH");
      AUTH_IP_Recive(fr);
      break;  
    default:
    LOG_ON("Unrecognized IPP");
  };
};

void IP_Receive(struct frame *frame){
  // Если некоректный пакет 
  if (!frame_filter(frame))
      return;
  
  // Заполняем метаданные
  fill_meta(frame);
  
  // Пакет предназначен для шлюза. Его нужно маршрутизировать
  if(frame->meta.FDST == 0){ 
    LOG_ON("Route to GW");
    RP_SendRT_GW(frame);
    return;
  };
  
  // Пакет для нас
  if (frame->meta.FDST == MODEL.node_adr || frame->meta.FDST == 0xffff){
    LOG_ON("IP frame for node. IPP_process");
    IPP_process(frame);
  };
  
  // Пакет для маршрутизации от шлюза к какомуто узлу
  RP_SendRT_RT(frame);
};

void IP_Send(struct frame *frame){
  struct IP_H iph;
  iph.ETX = MODEL.node_ETX;
  iph.FDST = 0; // Отправляем шлюзу
  iph.FSRC = MODEL.node_adr;
  iph.IPP = frame->meta.IPP;
     
  bool res =FR_add_header(frame, (char*)&iph, sizeof(struct IP_H));
  ASSERT(res);
  frame->meta.PID = PID_IP;
  LOG_ON("IP sended to gw");    
  RP_Send_GW(frame);    
};