#include "stdint.h"
#include "cmd_parser.h"
#include "model.h"
#include "debug.h"

enum {NET_NOSEEDING = 0, NET_SEEDING = 1};

/**
@brief Статус приемника
@detail Возвращает true, если идет раздача сети
*/
bool cmd_0x00(uint8_t *cmd, uint8_t size)
{
  uint8_t answer;
  
  if (MODEL.SYNC.mode !=0){
    answer = NET_SEEDING;
    LOG_ON("CMD 0x00. Seeding");
  }
  else{
    answer = NET_NOSEEDING;
    LOG_ON("CMD 0x00. No seeding");
  }
    
  cmd_answer(ATYPE_CMD_OK, &answer, sizeof(answer));
  return true;
}
