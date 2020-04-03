#include "stdbool.h"

int SL_zone_check();
int SL_available();
int SL_busy();
bool SL_free(char *buff);
char* SL_alloc(void);
bool SL_is_tx(char *buff);
bool SL_is_rx(char *buff);
void SL_set_tx(char *buff);
void SL_set_rx(char *buff);
char* SL_find_rx(char* buff);
char* SL_find_tx(char* buff);
int SL_rx_slots();
int SL_tx_slots();
void SW_restart();