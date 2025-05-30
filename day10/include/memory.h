#ifndef _MEMORY_H_
#define _MEMORY_H_

#define MEMMAN_ADDR 0x003c0000
#define MEMMAN_FREES 4090 // 大约是32kb

#include "asmfunc.h"

struct FreeInfo {
  unsigned int addr, size;
};

struct MemMan {
  int frees, maxfrees, lostsize, losts;
  struct FreeInfo free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);
//unsigned int memtest_sub(unsigned int start, unsigned int end);

void memman_init(struct MemMan *man);
unsigned int memman_total(struct MemMan *man);
unsigned int memman_alloc(struct MemMan *man, unsigned int size);
unsigned int memman_alloc_4K(struct MemMan *man, unsigned int size);
int memman_free(struct MemMan *man, unsigned int addr, unsigned int size);
int memman_free_4k(struct MemMan *man, unsigned int addr, unsigned int size);

#endif // _MEMORY_H_
