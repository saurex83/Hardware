#include "radio.h"
#include "mem_utils.h"
#include "debug.h"
#include "radio_defs.h"
#include "ustimer.h"
#include "frame.h"
#include "string.h"
#include "alarm_timer.h"
#include "model.h"
#include "action_manager.h"
#include "frame.h"
#include "macros.h"
#include "stdlib.h"

static void HW_Init(void);
static void SW_Init(void);
module_s RADIO_MODULE = {ALIAS(SW_Init), ALIAS(HW_Init)};

static void random_core_init(void);
static char RXBUFF[128];

#define IEEE_MODE 0     //!< Режимы фазы сигнала
#define NOT_IEEE_MODE 1 //!< Режимы фазы сигнала
#define RSSI_OFFSET -76 //!< Константа для вычисления реальной мощности
/*!
\details 
 Расчет качества связи. Коэффициенты получаются эксперементально.
 Есть предположение, что количество ошибок будет пропорционально квадрату
 коеффициента корреляции сигнала.
 LIQ = 255 передача 1000 пакетов без ошибок CRC
 LIQ = 127 передача 1000 пакетов с 50% ошибок CRC
 LIQ = 0   передача 1000 пакетов с 100% ошибок CRC
*/ 

#define CONST_A (int8_t)0 //!< Коэффициент A
#define CONST_B (int8_t)1 //!< Коэффициент B
#define LIQ_CALC(corr) ((corr-CONST_A)*CONST_B) //!< Формула вычисление LIQ 
#define FRQ_CALC(x) (11+5*(x-11)) //!< Формул вычисления частоты сигнала

#ifdef RADIO_DEBUG
  #define WRITE_TIME_DBG(where) (where) = AT_time()
  #define WRITE_PARA_DBG(where,val) (where) = (val) 
#else
  #define WRITE_TIME_DBG(where) {}
  #define WRITE_PARA_DBG(where,val)
#endif

static void SW_Init(void){
  // Настройки поумолчанию
  MODEL.RADIO.CRCError = 0;
  MODEL.RADIO.CCAReject = 0;
  MODEL.RADIO.channel = CH11;
  MODEL.RADIO.power_tx = m0x5;
  // Пост действия с радио
  random_core_init();
}

static void HW_Init(void){
  FRMFILT0 = 0x00; // Отключаем фильтрацию пакетов
  MDMTEST1_u MDM1; // Устанавливаем режим модуляции
  MDM1.value = MDMTEST1;
  MDM1.bits.MODULATION_MODE = IEEE_MODE;
  MDMTEST1 = MDM1.value;   
}

static inline void setFreq(channel_t ch)
{
  ASSERT( (ch >= MIN_CH) && (ch <= MAX_CH));
  // Устанавливаем частоту радиопередатчика
  FREQCTRL_u FRQ;
  FRQ.value = FRQ_CALC(MODEL.RADIO.channel);
  FREQCTRL = FRQ.value;  
}

static inline void pre_config(void){
  setFreq(MODEL.RADIO.channel); 
  TXPOWER = MODEL.RADIO.power_tx;
}

bool RI_SetChannel(channel_t channel){
  if (!(channel >= MIN_CH) && (channel <= MAX_CH))
    return false;
  MODEL.RADIO.channel = channel;
  return true;
}

static void LoadTXData(char *src, size_t len){
  ISFLUSHTX(); // Очищаем буфер передатчика
  RFD = len + 2; // Поле LEN на два байта больше
  for (size_t i = 0 ; i < len; i++)
    RFD = src[i];
  RFD = 0x00; // Добавляем CRC1,2
  RFD = 0x00;
};

static bool SendData(struct frame *fr){
  LoadTXData(fr->payload, fr->len);
  
  // Прежде чем включать радио нужно подождать
  if (fr->meta.SEND_TIME != 0) 
      AT_wait(fr->meta.SEND_TIME - 29);
  
  ISRXON();
  WRITE_TIME_DBG(MODEL.RADIO.DEBUG_TX.isrxon);
  RFIRQF1 &= ~RFIRQF1_TXDONE;
  RFIRQF0 &= ~RFIRQF0_SFD; 
  bool result = true;
  while(!RSSISTAT);
  WRITE_TIME_DBG(MODEL.RADIO.DEBUG_TX.rssistat);
  TRY{
    if (fr->meta.SEND_TIME != 0) // Отправка в определеное время
      AT_wait(fr->meta.SEND_TIME - 13); 
    ISTXONCCA();
    WRITE_TIME_DBG(MODEL.RADIO.DEBUG_TX.istxoncca);
    if (!(FSMSTAT1 & 1<<3)) //SAMPLED_CCA == 0
      THROW(1);
    // Ждем отправки SFD
    while (!(RFIRQF0 & RFIRQF0_SFD));
    fr->meta.TIMESTAMP = AT_time();
    WRITE_TIME_DBG(MODEL.RADIO.DEBUG_TX.sfd);
    // Ждем завершения отправки сообщения
    while (!(RFIRQF1 & RFIRQF1_TXDONE));
    WRITE_TIME_DBG(MODEL.RADIO.DEBUG_TX.txdone);
    
  }
  CATCH(1){ //SAMPLED_CCA == 0
    result = false;
  }
  FINALLY{
    ISRFOFF();
  }
  ETRY;  
  return result;
}

/*!
\brief Передает данные в эфир
*/
bool RI_Send(struct frame *fr){
  ASSERT(fr != NULL);
  pre_config();
  stamp_t start = UST_now();
  bool send_res = SendData(fr);
  stamp_t stop = UST_now();
  //TODO Неверно считает интервал так как при отправке
  // sync пакета мы ждем
  ustime_t tx_time = UST_interval(start, stop); 
  WRITE_PARA_DBG(MODEL.RADIO.DEBUG_TX.fulltime, tx_time);
  MODEL.RADIO.UptimeTX += tx_time;
  WRITE_PARA_DBG(MODEL.RADIO.DEBUG_TX.ccasampled, send_res);
  if (!send_res)
    MODEL.RADIO.CCAReject++;
  return send_res;
}


static void UnLoadRXData(char *src, size_t len){
  for (size_t i = 0 ; i < len; i++)
    src[i] = RFD;
};


static bool RecvData(ustime_t timeout, nwtime_t *sfd_stamp){   
  bool recv_result = true;
  
  ISFLUSHRX();
  RFIRQF0 &= ~RFIRQF0_RXPKTDONE;  
  RFIRQF0 &= ~RFIRQF0_SFD; 
  ISRXON();
  WRITE_TIME_DBG(MODEL.RADIO.DEBUG_RX.isrxon);
  stamp_t start = UST_now();
  TRY{
    while(true){
      if (UST_time_over(start, timeout))
        THROW(1);
      
      if ((RFIRQF0 & RFIRQF0_SFD)){ // Принят сигнал SFD
        *sfd_stamp = AT_time(); 
        WRITE_TIME_DBG(MODEL.RADIO.DEBUG_RX.sfd);
        break;
      }
    }
    // Ждем окончания приема пакета
    while (!(RFIRQF0 & RFIRQF0_RXPKTDONE));
    WRITE_TIME_DBG(MODEL.RADIO.DEBUG_RX.rxdone);
  }
  CATCH(1){ // Вышло время ожидания
    recv_result = false;
  }
  FINALLY{
    ISRFOFF();
  }
  ETRY;
  return recv_result;
}

/*!
\brief Принимает данные из эфира
\details Функция принимает данные из эфира. Проводит проверку CRC, увеличивает
RI_CRC_ERROR. Отмечает время прихода SFD в тактах сетевого времени .
\param[in] timeout Время ожидания данных в микросекундах
\return Возвращает NULL если данных нет
*/
struct frame* RI_Receive(ustime_t timeout){
  // Устанавливаем частоту передачи пакета
  pre_config();
  nwtime_t sfd_stamp;
     
  // Принимаем пакет 
  stamp_t start = UST_now();
  bool recv_res = RecvData(timeout, &sfd_stamp);
  stamp_t stop = UST_now();
  ustime_t rx_time = UST_interval(start, stop); 
  WRITE_PARA_DBG(MODEL.RADIO.DEBUG_RX.fulltime, rx_time);
  MODEL.RADIO.UptimeRX += rx_time;
  WRITE_PARA_DBG(MODEL.RADIO.DEBUG_RX.received, recv_res);
  if (!recv_res) // Нет пакета, возвращаем NULL
    return NULL;
  
  uint8_t frame_size = RXFIFOCNT; // Количество принятых данных
  
  // Минимальный размер FCS1,FCS2 - 2 байта. LEN не включен в подсчет 
  if (frame_size <= 2)
    return NULL;
  
  UnLoadRXData(RXBUFF, frame_size);
  
  int8_t  FCS1 = RXBUFF[frame_size - 2]; // RSSI
  uint8_t FCS2 = RXBUFF[frame_size - 1]; // bit7 = CRCOK, bit[6..0] Corr_val
  uint8_t LEN_F = RXBUFF[0]; // Поле LEN
  
  // Проверим поле LEN. Размер данных в заголовке должен совпадать
  // с фактической длинной принятых данных
  if (LEN_F != frame_size - 1){
    MODEL.RADIO.CRCError++;
    return NULL;
  }
  
  // Проверим поле CRCOK
  if (!(FCS2 && 1<<7)){
    MODEL.RADIO.CRCError++;
    return NULL;
  }
  
  // Создаем буфер, последнии два байта FCS1,2 и поле LEN не копируем
  struct frame *frame = FR_create();
  ASSERT(frame);
  bool add_h = FR_add_header(frame, &RXBUFF[1], frame_size - 3);
  ASSERT(add_h);
  
  frame->meta.TIMESTAMP = sfd_stamp;
  frame->meta.RSSI_SIG =  FCS1 + RSSI_OFFSET;
  uint8_t corr = FCS2 & 0x7F;
  frame->meta.LIQ = LIQ_CALC(corr);
  frame->meta.CH = MODEL.RADIO.channel;
  return frame;
}


/**
@breif Подссчет количества бит в байте
@return Возвращает количество бит
*/
static uint8_t bits_count(uint8_t value) {
  int ret = 0;
  for ( ; value; value = value >> 1 )
    ret += value & 1;
  return ret;
}

/**
@brief Возращает 1 байт случайного числа
@details Радио выдает всего лишь дви бита, нам нужно 8 
*/
static inline uint8_t getRNDByte(void)
{
  uint8_t val = 0;
  val |= RFRND << 0;
  val |= RFRND << 2;
  val |= RFRND << 4;
  val |= RFRND << 6;
  return val;
}

/**
@brief Возвращает случайное число с числом включеных бит от 2 до 6 включительно
@details Случайные числа появляются в регистре с частотой 170нс. Функция 
возвращает гарантированно новое значение. 
@return возвращает случайное число.
*/
static inline uint8_t readRandom(void)
{
  uint8_t rnd_val = 0;
  uint8_t bit_cnt = 0;
  while ( bit_cnt <= 2 || bit_cnt >= 6 )
  {
    rnd_val = getRNDByte();
    while (rnd_val == getRNDByte());
    rnd_val = getRNDByte();
    bit_cnt = bits_count(rnd_val);
  }
  return rnd_val;
}

/*!
\brief Иницилизирует ядро генератора случайных чисел данными из эфира
*/
static void random_core_init(void)  
{
  unsigned int rnd_core = 0;;
     
  pre_config();
  FREQCTRL = 0x00; // Выбираем не используемую частоту

  // TODO По какой то причине OP_EXE не выполняет команду.
  // регистра RFST читается как 0xD0. это его состояние при reset
  // Включаем демодулятор
  ISRXON();
  
  // Ждем пока статус RSSI_VALID станет true
  while(!RSSISTAT);
  
  // Ждем случайных чисел
  while (RFRND == 0);
  
  // Настраиваем ядро случайного генератора
  rnd_core = readRandom();
  rnd_core |= (unsigned int)readRandom()<<8;
  srand(rnd_core);
  
  // Выключаем демодулятор
  ISRFOFF();
  // Первая генерация случайного числа занимает много времени.
  // Влияло на работу радио, так как использовались случайные посылки
  rand(); 
}

/**
@brief Измерение мощности сигнала
@details Частота = 2394+fch. fch = [0..113]. 2394MHz to 2507MHz.
 Устройство поддерживает частоты до 2507 МГц
@param[in] fch номер ЧАСТОТНОГО канала
@param[in] timeout_ms время сканирования в мс
@param[out] Максимальная мощность сигнала за время сканирования
@return true если успешно
*/
bool RI_Measure_POW(char fch, ustime_t timeout, 
                    int8_t *RSSI_SIG){
  if (fch > 113)
    return false;
  //(2394+FREQCTRL.FREQ[6:0])
  FREQCTRL = fch; // Выбираем не используемую частоту
  ISRXON();
  while(!RSSISTAT);
  FRMCTRL0 |= 1<<4; // включаем ENERGY_SCAN, детектор пикового сигнала
  UST_delay(timeout);
  int8_t rssi = RSSI + RSSI_OFFSET;
  *RSSI_SIG = rssi;
  ISRFOFF();
  return true;
}

