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
@brief Установить системный канал. Сеть выкл.
Информация о системном канала распространяется в пакете синхронизации
*/
bool cmd_0x0F(uint8_t *cmd, uint8_t size)
{
  CHECK_SIZE();  
  CHECK_NETWORK_NOT_SEEDING();
  
  cmd_args_s *args = (cmd_args_s*)cmd; // Извлекаем аргументы
  MODEL.SYNC.sys_channel = args->CH;
  
  LOG_ON("CMD 0x0А. Set SYS CH");
  cmd_answer(ATYPE_CMD_OK, NULL, 0);
  return true;
}