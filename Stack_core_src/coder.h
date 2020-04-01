#pragma once
#include "stdbool.h"
#include "stdint.h"
#include "ustimer.h"
#include "global.h"

void AES_StreamCoder(bool enc_mode, char *src, char *dst, uint8_t len);
void AES_CCMEncrypt( uint8_t *src, uint8_t c, uint8_t f, uint8_t m, uint8_t *MIC);
bool AES_CCMDecrypt( uint8_t *src, uint8_t c, uint8_t f, uint8_t m, uint8_t *MIC);