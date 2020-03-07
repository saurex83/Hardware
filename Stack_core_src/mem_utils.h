#pragma once

struct memcpy {
  char *dst;
  char *src;
  char len;
};

struct memset {
  char *dst;
  char len;
  char val;
};

void MEM_memcpy(struct memcpy *memcpy);
void MEM_memset(struct memset *memset);

#define MEMCPY(DST, SRC, LEN) do {      \
    struct memcpy memcpy = {            \
      .src = (SRC),                     \
      .dst = (DST),                     \
      .len = (LEN)                      \
    };                                  \
    MEM_memcpy(&memcpy);                \
  } while(0)
    
#define MEMSET(DST, VAL, LEN) do {      \
    struct memset memset = {            \
      .len = (LEN),                     \
      .dst = (DST),                     \
      .val = (VAL)                      \
    };                                  \
    MEM_memset(&memset);                \
  } while(0)    