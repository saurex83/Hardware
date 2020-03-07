#pragma once
#include "stdbool.h"
#include "stdint.h"
#include "frame.h"
#include "ustimer.h"

typedef char channel_t;
typedef char power_t;

#define RADIO_DEBUG
#define MAX_CH  CH28
#define MIN_CH  CH11


#define RADIO_MODEL RADIO
struct RADIO{
  unsigned long CRCError;
  unsigned long CCAReject;
  power_t power_tx;
  channel_t channel;
  float UptimeTX;       // В секундах
  float UptimeRX;       // В секундах
#ifdef RADIO_DEBUG
  struct {
    nwtime_t isrxon;
    nwtime_t rssistat;
    nwtime_t istxoncca;
    nwtime_t sfd; 
    nwtime_t txdone;
    char ccasampled;
    ustime_t fulltime;
  } DEBUG_TX;
  struct {
    nwtime_t isrxon;
    nwtime_t sfd; 
    nwtime_t rxdone;
    ustime_t fulltime;
    char received;
  } DEBUG_RX;  
#endif  
};



enum CHANNAL_e
{
  CH11 = 11, CH12 = 12, CH13 = 13, CH14 = 14, CH15 = 15, CH16 = 16,
  CH17 = 17, CH18 = 18, CH19 = 19, CH20 = 21, CH22 = 22, CH23 = 23, 
  CH24 = 24, CH25 = 25, CH26 = 26, CH27 = 27, CH28 = 28
};

/*!
\brief Константы для установки выходной мощности радиопередатчика.
Пример: p4x5 = +4,5 дБ. m0x5 = -0.5 дБ
*/
enum TX_POWER_e 
{
  // +4.5дБ = p4x5, -1.5дБ = m1x5.
  // +4.5 = 34мА, +1 = 29мА, -4 = 26мА, -10 = 25 мА, -22 = 23 мА
  p4x5 = 0xF5, p2x5 = 0xE5, p1x0 = 0xD5, m0x5 = 0xC5, m1x5 = 0xB5, m3x0 = 0xA5,
  m4x0 = 0x95, m6x0 = 0x85, m8x0 = 0x75, m10x0 = 0x65, m12x0 = 0x55, 
  m14x0 = 0x45, m16x0 = 0x35, m20x0 = 0x15, m22x0 = 0x05
};

bool RI_SetChannel(channel_t CH);
bool RI_Send(struct frame *fr);
struct frame* RI_Receive(ustime_t timeout);
bool RI_Measure_POW(channel_t fch, ustime_t timeout_ms, int8_t *RSSI_SIG);
void RI_Reset(void);