#include "stdint.h"
#include "cmd_parser.h"
#include "model.h"
#include "llc.h"
#include "debug.h"

#define ARGS_SIZE sizeof(cmd_args_s)
typedef struct //!< Аргументы команды
{
  uint8_t TS;
  uint8_t CH;
  uint16_t crc16;
} cmd_args_s;

/**
@brief Открыть для приема слот TS на канале CH
*/
bool cmd_0x07(uint8_t *cmd, uint8_t size)
{
  CHECK_SIZE();  
  CHECK_NETWORK_SEEDING();
  // можно повесить трансивер если неверные параметры
  cmd_args_s *args = (cmd_args_s*)cmd; // Извлекаем аргументы
  
  LLC_open_slot(args->TS, args->CH);
  LOG_ON("CMD 0x07. Open slot");
  cmd_answer(ATYPE_CMD_OK, NULL, 0);
  return true;
}