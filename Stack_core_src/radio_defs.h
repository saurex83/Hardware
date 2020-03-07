#include "ioCC2530.h"

// Это Immediate Strobe instructions (ISxxx). Запись в командный регистр
// приводит к немедленному исполнению. Команды записываются в RFST.
// Состояние RFST можно не проверять. Он при выполнении ISxxx равен 0xD0
#define ISRXON()        RFST = 0xE3
#define ISTXON()        RFST = 0xE9
#define ISTXONCCA()     RFST = 0xEA
#define ISSAMPLECCA()   RFST = 0xEB
#define ISRFOFF()       RFST = 0xEF
#define ISFLUSHRX()     RFST = 0xED
#define ISFLUSHTX()     RFST = 0xEE


// Флаги прерывания RFIRQF0, RFIRQF1,
#define RFIRQF1_TXDONE      (1<<1)
#define RFIRQF0_RXPKTDONE   (1<<6)
#define RFIRQF0_SFD         (1<<1)

union MDMCTRL0_u
{
  uint8_t value;
  struct {
    uint8_t TX_FILTER:1;
    uint8_t PREAMBLE_LENGTH:4;
    uint8_t DEMOD_AVG_MODE:1;
    uint8_t DEM_NUM_ZEROS:2;
  } bits;
};

union MDMCTRL1_u
{
  uint8_t value;
  struct {
    uint8_t CORR_THR:5;
    uint8_t CORR_THR_SFD:1;
  } bits;
};

typedef union 
{
  uint8_t value;
  struct {
    uint8_t RESERVED:1;
    uint8_t MODULATION_MODE:1;
    uint8_t RFC_SNIFF_EN:1;
  } bits;
} MDMTEST1_u;

union FRMCTRL0_u
{
  uint8_t value;
  struct {
    uint8_t TX_MODE:2;
    uint8_t RX_MODE:2;
    uint8_t ENERGY_SCAN:1;
    uint8_t AUTOACK:1;
    uint8_t AUTOCRC:1;
    uint8_t APPEND_DATA_MODE:1;
  } bits;
};

union FRMCTRL1_u
{
  uint8_t value;
  struct {
    uint8_t SET_RXENMASK_ON_TX:1;
    uint8_t IGNORE_TX_UNDERF:1;
    uint8_t PENDING_OR:1;
  } bits;
};


union FREQTUNE_u // Можно слегка сместить частоту, чтобы нас не снифирелии
{
  uint8_t value;
  struct {
    uint8_t XOSC32M_TUNE:4;    
  } bits;
};


typedef union 
{
  uint8_t value;
  struct {
    uint8_t FREQ:7;    
  } bits;
} FREQCTRL_u;


union FSMSTAT0_u
{
  uint8_t value;
  struct {
    uint8_t FSM_FFCTRL_STATE:6;
    uint8_t CAL_RUNNING:1;   
  } bits;
};

typedef union 
{
  uint8_t value;
  struct {
    uint8_t RX_ACTIVE:1;
    uint8_t TX_ACTIVE:1;
    uint8_t LOCK_STATUS:1;
    uint8_t SAMPLED_CCA:1;
    uint8_t CCA:1;
    uint8_t SFD:1;
    uint8_t FIFOP:1;
    uint8_t FIFO:1;
  } bits;
} FSMSTAT1_u;

union CCACTRL1_u
{
  uint8_t value;
  struct {
    uint8_t CCA_HYST:3;
    uint8_t CCA_MODE:2;     
  } bits;
};

union RSSISTAT_u
{
  uint8_t value;
  struct {
    uint8_t RSSI_VALID:1;     
  } bits;
};

