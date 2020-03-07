#include "stdint.h"
#include "cmd_parser.h"
#include "model.h"
#include "debug.h"
#include "llc.h"

#define ARGS_SIZE sizeof(cmd_args_s)
typedef struct //!< Аргументы команды
{
  uint8_t TS;
  uint16_t crc16;
} cmd_args_s;

/**
@brief Закрыть для приема слот TS
*/
bool cmd_0x08(uint8_t *cmd, uint8_t size)
{
  CHECK_SIZE();  
  CHECK_NETWORK_SEEDING();
  
  cmd_args_s *args = (cmd_args_s*)cmd; // Извлекаем аргументы
  
  LLC_close_slot(args->TS);
  
  LOG_ON("CMD 0x08. Close TS slot");
  cmd_answer(ATYPE_CMD_OK, NULL, 0);
  return true;
}