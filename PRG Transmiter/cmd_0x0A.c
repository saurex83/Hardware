#include "stdint.h"
#include "cmd_parser.h"
#include "model.h"
#include "debug.h"
#include "buffer.h"

#define ARGS_SIZE sizeof(cmd_args_s)
typedef struct //!< Аргументы команды
{
  uint16_t crc16;
} cmd_args_s;

/**
@brief Количество пакетов в буфере TX. Сеть вкл.
*/
bool cmd_0x0A(uint8_t *cmd, uint8_t size)
{
  CHECK_SIZE();  
  CHECK_NETWORK_SEEDING();

  uint8_t nbItems = BF_tx_busy();
  cmd_answer(ATYPE_CMD_OK, &nbItems, sizeof(nbItems));
  
  LOG_ON("CMD 0x09. TX frame count");
  return true;
}