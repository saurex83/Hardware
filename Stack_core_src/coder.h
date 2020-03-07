#pragma once
#include "stdbool.h"
#include "stdint.h"
#include "ustimer.h"
#include "global.h"

#define AES_MODEL AES
struct AES{
  char  STREAM_KEY[16];
  char  STREAM_IV[16];
  char  CCM_KEY[16];
  char  CCM_IV[16];
  ustime_t elapsed_time;
};

void AES_StreamCoder(bool enc_mode, char *src, char *dst, uint8_t len);
void AES_CCMEncrypt( uint8_t *src, uint8_t c, uint8_t f, uint8_t m, uint8_t *MIC);
bool AES_CCMDecrypt( uint8_t *src, uint8_t c, uint8_t f, uint8_t m, uint8_t *MIC);