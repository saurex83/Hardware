#pragma once
#include "basic.h"

/**
@brief Главное хранилище данных и настроек узла.
@detail Не придумал ничего лучше как использовать глобальную структуру.
 Другие решения более сложные и обьемные. 
*/
typedef struct 
{
  uint8_t node_type;
  uint8_t node_ver;
  uint8_t stream_key[16];
  uint8_t stream_iv[16];
  uint8_t sync_channel;
  uint8_t sys_channel;
  uint8_t tx_power;
  uint8_t panid;
  uint8_t node_adr;
  uint8_t ts_slots[MAX_OPEN_SLOTS]; // =0 слот закрыт. Заполняет балансер
  uint8_t ch_slots[MAX_OPEN_SLOTS]; // =0 слот закрыт. Заполняет балансер
} database_s;

extern database_s CONFIG;

void CF_init();