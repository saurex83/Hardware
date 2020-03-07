#pragma once
#include "model_import_struct.h"

#define RED_ZONE 0x78

typedef struct {
    struct AM_MODEL AM;
    struct AT_MODEL AT;
    struct RTC_MODEL RTC;
    struct TM_MODEL TM;
    struct FR_MODEL FR;
    struct AES_MODEL AES;
    struct RADIO_MODEL RADIO;
    struct SYNC_MODEL SYNC;
    unsigned int node_adr;
} MODEL_s;


extern MODEL_s MODEL;