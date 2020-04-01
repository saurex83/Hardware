#pragma once
#include "meta.h"
#include "stdbool.h"

#define MAX_PAYLOAD_SIZE 127

struct frame{
  char len;
  struct meta meta;
  char payload[MAX_PAYLOAD_SIZE];
};

struct frame* FR_create();
bool FR_delete(struct frame*);
bool FR_add_header(struct frame*, void *head, char len);
bool FR_del_header(struct frame*, char len);
int FR_busy();
int FR_available();
int FR_rx_frames();
int FR_tx_frames();
void FR_set_rx(struct frame* frame);
void FR_set_tx(struct frame* frame);
bool FR_is_rx(struct frame* frame);
bool FR_is_tx(struct frame* frame);
struct frame* FR_find_tx(struct frame* frame);
struct frame* FR_find_rx(struct frame* frame);

