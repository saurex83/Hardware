#include "stdint.h"
#include "cmd_parser.h"
#include "model.h"
#include "string.h"
#include "debug.h"

#define ARGS_SIZE sizeof(cmd_args_s)
typedef struct //!< Аргументы команды
{
  uint8_t IV[16];
  uint16_t crc16;
} cmd_args_s;

/**
@brief Загрузка IV
*/
bool cmd_0x04(uint8_t *cmd, uint8_t size)
{
  CHECK_SIZE();  
  CHECK_NETWORK_NOT_SEEDING();
   
  cmd_args_s *args = (cmd_args_s*)cmd; // Извлекаем аргументы
  memcpy(MODEL.AES.STREAM_IV,  args->IV, 16);   
 
  LOG_ON("CMD 0x04. IV loaded");
  
  cmd_answer(ATYPE_CMD_OK, NULL, 0);
  return true;
}