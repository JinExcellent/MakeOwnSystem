#ifndef _CMD_H_
#define _CMD_H_
#include "console.h"

void cmd_mem(struct CONSOLE *cons, unsigned int memtotal);
void cmd_cls(struct CONSOLE *cons);
void cmd_dir(struct CONSOLE *cons);
void cmd_type(struct CONSOLE *cons, int *fat, char *cmdline);
void cmd_hlt(struct CONSOLE *cons, int *fat);

#endif
