#pragma once

struct vtimer{
  unsigned long last;
  unsigned int exp_time;
};

void vtimer_set(struct vtimer *vt, unsigned int sec);
void vtimer_start(struct vtimer *vt);
bool vtimer_expired(struct vtimer *vt); 