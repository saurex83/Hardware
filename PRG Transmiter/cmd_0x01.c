#include "stdint.h"
#include "cmd_parser.h"
#include "model.h"
#include "debug.h"

#define ARGS_SIZE sizeof(cmd_args_s)
typedef struct //!< Аргументы команды
{
  uint8_t panid;
  uint16_t crc16;
} cmd_args_s;

/**
@brief Установить panid
*/
bool cmd_0x01(uint8_t *cmd, uint8_t size)
{
  CHECK_SIZE();  
  CHECK_NETWORK_NOT_SEEDING();
  
  cmd_args_s *args = (cmd_args_s*)cmd; // Извлекаем аргументы
  
  MODEL.SYNC.panid =args->panid;
  LOG_ON("CMD 0x01. Set panid");
  
  cmd_answer(ATYPE_CMD_OK, NULL, 0);
  return true;
}