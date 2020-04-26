#pragma once
#include "frame.h"

void UDP_Recive(struct frame *frame);
bool UDP_bind_port(char port, void (*fn)(struct frame *frame));
void UDP_Send(char port, char *ptr, char size);