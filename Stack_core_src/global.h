#include "radio.h"
//C compiler options->extra options add  --no_path_in_file_macros

#define DEBUG
#define PRINT_TO_TERMINAL
#define SIG_DEBUG

#define NODE_VER 0
#define NODE_TYPE 1
#define HEADER_ETH_VER 1 // Версия протокола

#define DEFAULT_STREAM_KEY  {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}
#define DEFAULT_STREAM_IV   {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}
#define DEFAULT_CCM_KEY  {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}
#define DEFAULT_CCM_IV   {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16}

#define DEFAULT_SYNC_CH CH28
#define DEFAULT_SYS_CH CH11

#define FRAME_FOOTER_DEL //!< FR_del_header заполняет пустое место нулями
//#define FILL_SLOT_ZERO //!< SL_alloc заполняет буфер нулями. Если выкл. прог виснет на addHeader
 
#define MAX_PACKETS 20 
#define MAX_RX_BUFF     10
#define MAX_TX_BUFF     10
#define MAX_TXRX_BUFF_SIZE MAX_PACKETS //!< Кол-во пакетов в буфере
#define SLOT_POOL_ITEMS MAX_PACKETS //!< Количество слотов памяти в системе
#define FRAME_POOL_ITEMS_COUNT MAX_PACKETS //!< Количество фреймов в системе
#define MAX_TXRX_BUFFER MAX_PACKETS //!< Размер буфера