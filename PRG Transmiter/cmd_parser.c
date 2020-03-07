#include "uart_comm.h"
#include "stdbool.h"
#include "cmd_index_list.h"
#include "debug.h"
#include "cmd_parser.h"

/**
@file Прием и парсинг команд по uart
@detail Команда от PC имеет формат:
 --------------------------
 | 0 | 1 |  n |LEN-1| LEN | 
 --------------------------
 |LEN|CMD|ARGS|CRC16|CRC16|
 --------------------------
Ответ от МК имеет формат:
 --------------------------
 | 0 | 1 |  n |LEN-1| LEN | 
 --------------------------
 |LEN|TYP|DATA|CRC16|CRC16|
 --------------------------
Где TYP - тип ответа.
TYP = 0x00 ответ от парсера в случаии ошибки.
 DATA = 1 ошибка CRC16
 DATA = 2 команда отсутсвует
TYP = 0x01 ответ от обработчика команды с ошибкой
 DATA = 0x01 Ошибка размера аргументов
 DATA = 0x02 Аргумент неверен.
TYP = 0x02 Команда выполнена, следуют данные команды
*/

enum {answ_crc_err = 1, answ_args_err = 2, answ_no_cmd = 3}; //!< Коды ошибок

//#define USE_CRC16_CHECK //!< Использовать проверку CRC
#define MAX_CMD_INDEX sizeof(CMD_LIST) //!< Все номера команд меньше этого числа

static unsigned short crc16(unsigned char *pcBlock, unsigned short len);
static bool check_cmd_frame(uint8_t *cmd, uint8_t size);
void cmd_answer(uint8_t ATYPE, uint8_t *data, uint8_t len);
void cmd_answer_err(uint8_t a_type, uint8_t err_code);

typedef bool(*cmd_handler)(uint8_t *cmd, uint8_t size);

// Список обработчиков. Номер команды = индекс в массиве
static cmd_handler CMD_LIST[] = { 
  cmd_0x00, cmd_0x01, cmd_0x02, cmd_0x03, cmd_0x04, cmd_0x05, cmd_0x06,
  cmd_0x07, cmd_0x08, cmd_0x09,cmd_0x0A, cmd_0x0B, cmd_0x0C, cmd_0x0D,
  cmd_0x0E, cmd_0x0F, cmd_0x10
};    
 


/**
@brief Разбор входящих команд от PC
@detail После приема следует ответ если есть ошибки CRC, args, len.
 Если ошибок нет, то ответ формирует команда.
*/
void parse_uart_stream(void)
{
  uint8_t* cmd;
  uint8_t size;
  uint8_t iCmd;
  
  while (true){
    cmd = uart_recv_cmd(&size);
    if (!check_cmd_frame(cmd,size)){ // Кадр плохой
      cmd_answer_err(ATYPE_PAR_ERR, PAR_CRC16);
      continue; 
    }
      
    iCmd = cmd[0];
    if (!(iCmd < MAX_CMD_INDEX)){ // Нет такой команды
      cmd_answer_err(ATYPE_PAR_ERR, PAR_NOCMD);
      continue; 
    }
    
    if (!CMD_LIST[iCmd](&cmd[1], size - 1)) 
      continue; // если были ошибки в выполнении команды.
    
    // Впринципе проверять не нужно как завершилась команда, но на будующее 
    // оставлю этот функционал. 
    // Сюда приходим если нет проблем с командой
  }
}

/**
@brief Ответ на команду с ошибкой
@param[in] ATYPE тип ответа
@param[in] data указатель на данные
@param[in] len размер данных
*/
void cmd_answer(uint8_t ATYPE, uint8_t *data, uint8_t len){
  // Отдаем заголовок
  uint8_t full_len = len+3; // Байт TYPE + 2б CRC16
  uint8_t an[2] = {full_len, ATYPE};
  uart_write((char*)an, sizeof(an));
  
  // Отдаем данные
  uart_write((char*)data, len);
  
  unsigned short an_crc16 = crc16(data,len);
  uart_write((char*)an_crc16, sizeof(an_crc16));  
}

void cmd_answer_err(uint8_t a_type, uint8_t err_code){
  // Отдаем заголовок
  uint8_t answ_err[] = {a_type, err_code};
  stream_write(answ_err, sizeof(answ_err));
}

/**
@brief Отправляем данные в PC. 
@brief Формирует байт размера и добавляет CRC16
*/
void stream_write(uint8_t *data, uint8_t size)
{
  ASSERT(size > 0 && size < 253) // +2 CRC +1 LEN +252 DATA = 255 максимум
  uint8_t nsize = size + 2; // Два байта CRC
  unsigned short data_crc16 = crc16(data, size);
  uart_write((char*)&nsize, 1);
  uart_write((char*)data, size);
  uart_write((char*)&data_crc16 , 2);
}

/**
@brief Проверка коректности принятого пакета
*/
static bool check_cmd_frame(uint8_t *cmd, uint8_t size)
{  
  if (size < 3) // Размер данных не может быть меньше 3 байт
      return false;
    
#ifdef USE_CRC16_CHECK    
  unsigned short* cmd_crc16;
  unsigned short data_crc16;  
  // Извлекаем CRC16 из пакета (последнии 2 байта) 
  cmd_crc16 = (unsigned short*)&cmd[size - 2];
  // Расчитываем CRC16 данных
  data_crc16 = crc16(cmd, size - 2);
  // Если данные битые то продолжаем прием
  if (data_crc16 != *cmd_crc16)
    return false;
#endif

  return true;
}

/**
@brief Расчет CRC16
@param[in] pcBlock указатель на начало масива
@param[in] len размер данных для подсчета
@return код crc16
*/
static unsigned short crc16(unsigned char *pcBlock, unsigned short len){
    unsigned short crc = 0xFFFF;
    unsigned char i;
 
    while (len--)
    {
        crc ^= *pcBlock++ << 8;
 
        for (i = 0; i < 8; i++)
            crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
    }
 
    return crc;
}
