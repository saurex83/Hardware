#include "stdint.h"
#include "cmd_parser.h"
#include "model.h"
#include "debug.h"

#define ARGS_SIZE sizeof(cmd_args_s)
typedef struct //!< Аргументы команды
{
  uint8_t dont_use;
  uint16_t crc16;
} cmd_args_s;

/**
@brief перезарузка
*/
bool cmd_0x06(uint8_t *cmd, uint8_t size)
{
  CHECK_SIZE();  
   
  cmd_answer(ATYPE_CMD_OK, NULL, 0);
  LOG_ON("CMD 0x06. HW reload");
  //TODO АППАПРАТНАЯ перезагрузка.
  
  return true;
}