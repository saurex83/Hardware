#include "stdint.h"
#include "cmd_parser.h"
#include "model.h"
#include "debug.h"

static void upload_frame(struct frame *fr);
static void mem_move(uint8_t *dst, uint8_t *src, uint8_t len);

#define ARGS_SIZE sizeof(cmd_args_s)
typedef struct //!< Аргументы команды
{
  uint16_t crc16;
} cmd_args_s;


static uint8_t upload[sizeof(struct meta) + 2 + 127];

/**
@brief Отправить на PC принятый пакет из буфера RX. Сеть вкл.
*/
bool cmd_0x0C(uint8_t *cmd, uint8_t size)
{
  CHECK_SIZE();  
  CHECK_NETWORK_SEEDING();
  
  struct frame *rx_frame = NULL;
  rx_frame = FR_find_rx(rx_frame);
  
  if (!rx_frame){
    cmd_answer_err(ATYPE_CMD_ERR, CMD_RX_EMPTY);
    return false;
  }
  
  upload_frame(rx_frame);
  
  FR_delete(rx_frame);
  LOG_ON("CMD 0x0C. Upload RX frame");
  return true;
}

/**
@brief Выгрузить фрейм в uart
@detail Порядок выгрузки meta, byte len, payload[len]
*/
static void upload_frame(struct frame *fr){
/*  Заполнение массива upload
          -----------------------
          |META|LEN|   PAYLOAD  |
          -----------------------
               ^   ^<- fr.len ->^
sizeof(meta_s)-|   |
sizeof(meta_s)+1  -| 
*/
  // Пакет буду собирать в массиве upload иначе crc считать неудобно
   // Размер мета+1 байт длинны+пакет
  uint8_t answ_len = sizeof(struct meta) + 1 + fr->len; // Суммарный размер пакета
  mem_move(upload, (uint8_t*)&fr->meta, sizeof(struct meta));
  upload[sizeof(struct meta)] = fr->len; // byte len
  mem_move(&upload[sizeof(struct meta) + 1], (uint8_t*)fr->payload, fr->len);
  
  cmd_answer(ATYPE_CMD_OK, upload, answ_len);
  //cmd_answer(ATYPE_CMD_OK, upload, 10);
}

static void mem_move(uint8_t *dst, uint8_t *src, uint8_t len){
  for (uint8_t i = 0; i < len; i++)
    dst[i] = src[i];
}