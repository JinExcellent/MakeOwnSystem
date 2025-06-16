#include "cmd.h"
#include <include/console.h>


void cmd_mem(struct CONSOLE *cons, unsigned int memtotal){
    struct MemMan *memman = (struct MemMan *) MEMMAN_ADDR;
    char s[60];
    sprintf(s, "total %dMB\nfree %dKB\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    cons_putstr(cons, s);
    
    //sprintf(s, "free %dMB", memman_total(memman) / 1024);
   // cons_putstr(cons, s);
}

void cmd_cls(struct CONSOLE *cons){
    struct Sheet *sheet = cons->sht;
    for(int y = 28; y < 28 + 128; y++){
        for(int x = 8; x < 8 + 240; x++)
            sheet->buf[x + y * sheet->bxsize] = COL8_000000;
    }

    sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
    cons->cursor_y = 28;
}

void cmd_dir(struct CONSOLE *cons){
    char s[30];
    struct FILEINFO *finfo = (struct FILEINFO *)(ADR_DISKIMG + 0x002600);
    for(int x = 0; x < 224; x++){
        if(finfo[x].name[0] == 0x00){
            break;
        }
        if(finfo[x].name[0] != 0xe5){
            if (!(finfo[x].type & 0x18)) {
                sprintf(s, "filename.ext %7d\n", finfo[x].size);
                for(int y = 0; y < 8; y++) {
                    s[y] = finfo[x].name[y];
                }
                s[9] = finfo[x].ext[0];
                s[10] = finfo[x].ext[1];
                s[11] = finfo[x].ext[2];
                
                cons_putstr(cons, s);
            }
        }   
    }
    cons_newline(cons);
}

void cmd_type(struct CONSOLE *cons, int *fat, char *cmdline){
    struct MemMan *memman = (struct MemMan *)MEMMAN_ADDR;
    struct FILEINFO *finfo = file_search(cmdline + 5, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
    char *p;

    if(finfo != 0){
    //找到文件
        p = (char *)memman_alloc_4K(memman, finfo->size);
        file_load_file(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
        /*for(int i = 0; i < finfo->size; i++){
           cons_putchar(cons, p[i], 1); 
        }*/
        cons_putnstr(cons, p, finfo->size);
        memman_free_4K(memman, (int)p, finfo->size);
    }else{      //没有找到文件
        cons_putstr(cons, "not found file.\n");
    }
    cons_newline(cons);
}

void cmd_hlt(struct CONSOLE *cons, int *fat){
	struct MemMan *memman = (struct MemMan *) MEMMAN_ADDR;
	struct FILEINFO *finfo = file_search("hlt.hrb", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	struct SegmentDescriptor *gdt = (struct SegmentDescriptor *) ADR_GDT;
	char *p;
	if (finfo != 0) {
		/*找到文件的情况*/
		p = (char *) memman_alloc_4K(memman, finfo->size);
		file_load_file(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
		set_segmdesc(gdt + 1003, finfo->size - 1, (int) p, AR_CODE32_ER);
		far_call(0, 1003 * 8);
		memman_free_4K(memman, (int) p, finfo->size);
	} else {
		/*没有找到文件的情况*/
		put_font8_asc_sht(cons->sht, 8, cons->cursor_y, COL8_FFFFFF, COL8_000000, "File not found.", 15);
		cons_newline(cons);
	}
	cons_newline(cons);
	return;
}
