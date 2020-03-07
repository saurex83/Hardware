#include "stdint.h"
#include "cmd_parser.h"
#include "model.h"
#include "debug.h"

#define ARGS_SIZE sizeof(cmd_args_s)
typedef struct //!< Аргументы команды
{
  bool on_off;
  uint16_t crc16;
} cmd_args_s;

/**
@brief Включить или выключить сеть. true - вкл
*/
bool cmd_0x03(uint8_t *cmd, uint8_t size)
{
  CHECK_SIZE();
  
  cmd_args_s *args = (cmd_args_s*)cmd; // Извлекаем аргументы

  if (args->on_off){ // Просят включить сеть
    MODEL.SYNC.mode = 2;
    MODEL.TM.MODE = 2;
    Neocore_start();    
    //network_seed_enable(true);
    LOG_ON("CMD 0x03. Start network");
  }
  else
  {
    MODEL.SYNC.mode = 0;
    MODEL.TM.MODE = 0;
    LOG_ON("CMD 0x03. Stop network");
  }
  
  cmd_answer(ATYPE_CMD_OK, NULL, 0);
  return true;
}