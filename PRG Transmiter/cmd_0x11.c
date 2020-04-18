#include "stdint.h"
#include "cmd_parser.h"
#include "model.h"
#include "debug.h"

#define ARGS_SIZE sizeof(cmd_args_s)
typedef struct //!< Аргументы команды
{
  uint16_t crc16;
} cmd_args_s;

/**
@brief Возвращает энергию в каналах
*/
bool cmd_0x11(uint8_t *cmd, uint8_t size)
{
  CHECK_SIZE();  
  CHECK_NETWORK_SEEDING();

  cmd_answer(ATYPE_CMD_OK, (uint8_t*)MODEL.PWR_SCAN.energy, sizeof(MODEL.PWR_SCAN.energy));
  for (int i = 0; i < sizeof(MODEL.PWR_SCAN.energy); i++)
    MODEL.PWR_SCAN.energy[i] = -127;
  
  LOG_ON("CMD 0x11. Energy scan info");
  return true;
}