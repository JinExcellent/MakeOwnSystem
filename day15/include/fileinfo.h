#ifndef _FILEINFO_H_
#define _FILEINFO_H_

struct FILEINFO{
    unsigned char name[8], ext[3], type;    //文件名，文件类型，文件属性
    char reserve[10];                       //保留
    unsigned short time, data, clustno;     //clustno文件所在扇区
    unsigned int size;                      //文件大小
};

void file_read_fat(int *fat, unsigned char *img);
void file_load_file(int clustno, int size, char *buf, int *fat, char *img);

#endif
