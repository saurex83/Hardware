#pragma once
#include "action_manager.h"
#include "stdlib.h"

/* Перечень груп в которые входят модули */
#define COLD_MOD_DEF {&SYNC_MODULE, &LLC_MODULE, &LED_MODULE}

#define HOT_MOD_DEF {&RTC_MODULE, &SYNC_MODULE, &LLC_MODULE, &LED_MODULE}

#define HW_MOD_DEF {&CPU_MODULE, &DBG_MODULE, &AT_MODULE, &UST_MODULE,  \
  &DMA_MODULE, &CODER_MODULE, &RADIO_MODULE, &LED_MODULE}

#define SW_MOD_DEF {&FR_MODULE, &MD_MODULE, &MS_MODULE, \
  &BF_MODULE, &RADIO_MODULE, &SYNC_MODULE, &LLC_MODULE, \
  &MAC_MODULE}

#define IRQ_MOD_DEF {&BF_MODULE, &MAC_MODULE, &LLC_MODULE}

/* Обьявления всех модулей */
extern module_s  AT_MODULE, FR_MODULE, RTC_MODULE, UST_MODULE,
  MD_MODULE, DBG_MODULE, CPU_MODULE, MS_MODULE, BF_MODULE,
  DMA_MODULE, CODER_MODULE, RADIO_MODULE, SYNC_MODULE, RTC_MODULE,
  LLC_MODULE, LED_MODULE, MAC_MODULE;


//#include "action_manager.h"
//static void SW_Init(void);
//module_s BF_MODULE = {ALIAS(SW_Init)};
