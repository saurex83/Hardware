#pragma once
#include "stdint.h"
#include "RADIO.h"
//C compiler options->extra options add  --no_path_in_file_macros
#define GATEWAY
#define NODE_VER 0
#define NODE_TYPE 1

#define HEADER_ETH_VER 1 // Версия протокола

#define RARIO_STREAM_ENCRYPT  // Шифрование данных включенно 

#define DEFAULT_KEY  {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}
#define DEFAULT_IV   {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}
#define DEFAULT_SYNC_CHANNEL CH28
#define DEFAULT_SYS_CHANNEL  CH11
#define DEFAULT_PANID 0x12 
#define DEFAULT_TX_POWER  m22x0    //m0x5 

#define MAX_OPEN_SLOTS 3 // Максимальное число временых слотов для приема
#define NEIGHBOR_ALIVE_TIME 600 // Время жизни записи в табл. соседей в сек.
#define NEIGHBOR_CARD_SEND_INTERVAL 180 // Интервал передачи инф. об узле в сек.
#define NEIGHBOR_CARD_SEND_INTERVAL_DEV 20 // Случайное отклонение интервала 
#define NEIGHBOR_CARD_REQ_INTERVAL  30 // Как часто можно отправлять запрос карт
#define NEIGHBOR_CARD_REQ_INTERVAL_DEV  10 // Случайное отклонение
#define NEIGHBOR_THR_RECV_TIME 220 // Спустя это время, узел начнет запрашивать соседей

#define ROUTE_TABLE_ITEMS 20 // Количество маршрутов в таблице маршрутизации

#define TB_START_DELAY 120 // После этого времени узел начнет открывать свои слоты
#define TB_START_DEV 20 // Случайное отклонение