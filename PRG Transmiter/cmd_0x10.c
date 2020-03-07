#include "stdint.h"
#include "cmd_parser.h"
#include "model.h"
#include "debug.h"

#define ARGS_SIZE sizeof(cmd_args_s)
typedef struct //!< Аргументы команды
{
  uint8_t POW;
  uint16_t crc16;
} cmd_args_s;

/**
@brief Установить мощность радиопередатчика
*/
bool cmd_0x10(uint8_t *cmd, uint8_t size)
{
  CHECK_SIZE();
  CHECK_NETWORK_NOT_SEEDING();
    
  cmd_args_s *args = (cmd_args_s*)cmd; // Извлекаем аргументы
  MODEL.RADIO.power_tx = args->POW;
  
  LOG_ON("CMD 0x10. Set TX POWER");
  cmd_answer(ATYPE_CMD_OK, NULL, 0);
  return true;
}