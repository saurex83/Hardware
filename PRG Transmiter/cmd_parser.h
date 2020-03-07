#include "stdint.h"

// Типы ответов трансивера
enum {ATYPE_PAR_ERR = 0, ATYPE_CMD_ERR = 1, ATYPE_CMD_OK = 2};
// Ошибки парсера
enum {PAR_CRC16 = 1, PAR_NOCMD = 2};
// Ошибки команды
enum {CMD_LEN = 1, CMD_ARG_VAL = 2, CMD_SEEDING = 3, CMD_NOSEEDING = 4,
  CMD_TX_FULL = 5, CMD_RX_EMPTY = 6 };

void parse_uart_stream(void);
void stream_write(uint8_t *data, uint8_t size);
void cmd_answer(uint8_t ATYPE, uint8_t *data, uint8_t len);
void cmd_answer_err(uint8_t a_type, uint8_t err_code);

#define CHECK_SIZE()                            \
    if (size != ARGS_SIZE){                     \
      cmd_answer_err(ATYPE_CMD_ERR, CMD_LEN);   \
      return false;                             \
    }
    
#define CHECK_NETWORK_SEEDING()                         \
    if (MODEL.SYNC.mode == 0){                          \
      cmd_answer_err(ATYPE_CMD_ERR, CMD_NOSEEDING);     \
      return true;                                      \
    }    
    
#define CHECK_NETWORK_NOT_SEEDING()                     \
    if (MODEL.SYNC.mode != 0){                          \
      cmd_answer_err(ATYPE_CMD_ERR, CMD_SEEDING);       \
      return true;                                      \
    }     