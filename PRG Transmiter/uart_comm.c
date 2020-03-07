#include "debug.h"
#include "ioCC2530.h"
#include "stdint.h"
#include "stdbool.h"
#include "DMA.h"

/**
@file Настройка UART для приема команд от PC
*/

#define UART_RX_BUF_LEN 0xff //!< Размер приемного буфера
#define TRIG_URX1 16

static void uart_dma_init(void);

void uart_write(const char *data, uint8_t size);
uint8_t* uart_recv_cmd(uint8_t *size);

static uint8_t uart_rx_buff[UART_RX_BUF_LEN]; //1 для поля длинны
static void uart_putchar(char x);

static void uart_dma_init(void)
{
  // Настроим канал 2 DMA для загрузки данных из uart
  // DMA0 и 1 заняты кодером AES
  
  ST_DEF(DMA_CH[2], SRCADDRL, 0xF9); //U1DBUF
  ST_DEF(DMA_CH[2], SRCADDRH, 0x70);
  ST_DEF(DMA_CH[2], LENL, UART_RX_BUF_LEN); // Максимальный размер данных
  ST_DEF(DMA_CH[2], DSTADDRH, HADDR(uart_rx_buff)); // Пишем данные XENCDI = 0xB1 
  ST_DEF(DMA_CH[2], DSTADDRL, LADDR(uart_rx_buff)); //  
  ST_DEF(DMA_CH[2], PRIORITY, 0x00); // Низкий приоритет
  ST_DEF(DMA_CH[2], M8, 0x00); // Используем 8 бит для счетика длинны
  ST_DEF(DMA_CH[2], IRQMASK, 0x00); // Запрещаем генерировать перывания
  ST_DEF(DMA_CH[2], DESTINC, 0x01); // Увеличиваем адресс назначения
  ST_DEF(DMA_CH[2], SRCINC, 0x00); // Не увеличиваем адресс источника
  ST_DEF(DMA_CH[2], TRIG, TRIG_URX1); // Тригер по приходу байта UART0
  ST_DEF(DMA_CH[2], WORDSIZE, 0x00); // Копируем по 1 байту
  ST_DEF(DMA_CH[2], TMODE, 0x00); //  Одиночное копирование по тригеру
  ST_DEF(DMA_CH[2], VLEN, 0x01); // Длинна данных определяется 1ым байтом  
  ST_DEF(DMA_CH[2], LENH, 0x00); 
}

/**
@brief Прием команды по uart
@param[out] size размер принятых данных
@return указатель на начало данных или null
*/
uint8_t* uart_recv_cmd(uint8_t *size)
{  
  // Начинаем прием
  DMAARM |= 0x04;
  
  while (DMAARM&0x04);
  
  *size = uart_rx_buff[0];
  return &uart_rx_buff[1];
}

/**
@brief Иницилизация uart с компьютером
@detail Скорость 115200 бод. Частота микроконтроллера 32Мгц.
*/
void com_uart_init(void)
{
  // Выбор расположения UART на выводах
  uint8_t U1CFG = 1; // 0={TX=P0_4, RX=P0_5 Локация 1. 1={P1_6=TX P1_7=RX}
  PERCFG |= (U1CFG<<1); 
  
  // U1UCR регистр настройки режима uart. меня устраивает по умолчанию
  U1CSR = (1<<7); // Выбираем режим uart  
  
  // Настройка скорости передачи данных на 115200  
  U1BAUD = 216;  // табличные значения из pdf
  U1GCR =  11; //16
  
  // Включаем альтернативные функции выводов
  P1SEL |= (1<<6)|(1<<7);
  U1CSR |= (1<<6);
  uart_dma_init();
}

/**
@brief Запись в uart
@param[in] data указатель на начало массива данных
@param[in] size размер передаваемых данных
*/
void uart_write(const char *data, uint8_t size)
{
  for (uint8_t i = 0; i < size; i++)
    uart_putchar((char)data[i]);
};


static void uart_putchar(char x){
  while( U1CSR&(1<<0));
  U1DBUF = x;
}

 char uart_getchar(void)
{
  char x;
  while( !(U1CSR&(1<<2)));
  x = U1DBUF;
  return x;
}
