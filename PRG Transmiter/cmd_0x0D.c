#include "stdint.h"
#include "cmd_parser.h"
#include "model.h"
#include "RADIO.h"
#include "debug.h"

#define ARGS_SIZE sizeof(cmd_args_s)
typedef struct //!< Аргументы команды
{
  uint8_t FCH;
  uint16_t MS;
  uint16_t crc16;
} cmd_args_s;

/**
@brief Скан канала CH в течении time_ms. Сеть выкл.
@detail Ответ после завершения сканирования
*/
bool cmd_0x0D(uint8_t *cmd, uint8_t size)
{
  CHECK_SIZE();  
  CHECK_NETWORK_NOT_SEEDING();
  
  cmd_args_s *args = (cmd_args_s*)cmd; // Извлекаем аргументы
  
  int8_t rssi;
  bool res = RI_Measure_POW(args->FCH, args->MS, &rssi);
  
  LOG_ON("CMD 0x0D. RSSI scan");
  
  cmd_answer(ATYPE_CMD_OK, (uint8_t*)&rssi, sizeof(rssi));
  return true;
}