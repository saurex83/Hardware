#include "model.h"
#include "vtimer.h"

void vtimer_set(struct vtimer *vt, unsigned int sec){
  vt->exp_time = sec;
  vt->last = MODEL.RTC.uptime;
};

void vtimer_start(struct vtimer *vt){
  vt->last = MODEL.RTC.uptime;
};

bool vtimer_expired(struct vtimer *vt){
  unsigned long now = MODEL.RTC.uptime;
  if ((now - vt->last) < vt->exp_time)
    return false;
  return true;
};