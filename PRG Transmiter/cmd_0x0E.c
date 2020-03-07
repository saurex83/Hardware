#include "stdint.h"
#include "cmd_parser.h"
#include "model.h"
#include "debug.h"

#define ARGS_SIZE sizeof(cmd_args_s)
typedef struct //!< Аргументы команды
{
  uint8_t CH;
  uint16_t crc16;
} cmd_args_s;

/**
@brief Установить канал для синхросигнала. Сеть выкл.
*/
bool cmd_0x0E(uint8_t *cmd, uint8_t size)
{
  CHECK_SIZE();  
  CHECK_NETWORK_NOT_SEEDING();
  
  cmd_args_s *args = (cmd_args_s*)cmd; // Извлекаем аргументы
  MODEL.SYNC.sync_channel = args->CH;
  
  LOG_ON("CMD 0x0E. Set SYNC CH");
  cmd_answer(ATYPE_CMD_OK, NULL, 0);
  return true;
}