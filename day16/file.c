#include "fileinfo.h"

/*
 *该函数如果无法理解可以通过举例：
 *  FAT12中三个字节表示两个簇号（12位一个簇号）
 *  假如由三个字节 03 40 00
 *  通过以下逻辑移位和与运算可以得到两个簇号分别为 003 004
 * */
void file_read_fat(int *fat, unsigned char *img){
    for(int i = 0, j = 0; i < 2880; i+=2){
        fat[i + 0] = (img[j + 0] | img[j + 1] << 8) & 0xfff;
        fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;

        j += 3;
    }
}

void file_load_file(int clustno, int size, char *buf, int *fat, char *img){
    while(1){
        //当正好一个或小于一个块大小，只读取剩余size个字符
        if(size <= 512){
            for(int i = 0; i < size; i++)
                buf[i] = img[clustno * 512 + i];
            break;
        }
        for(int i = 0; i < 512; i++){
            buf[i] = img[clustno * 512 + i];
        }

        size -= 512;    //计算剩余带读取字节数
        buf += 512;     //每读完一个块，向后偏移
        clustno = fat[clustno];     //读取下一个块号
    }

}

struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max){
    char s[12];
    int j;
    //准备文件名
    for(int i = 0; i < 11; i++)
        s[i] = ' ';
    j = 0;
    for(int i = 0; name[i] != 0; i++){
        if(j >= 11) 
            return 0;

        if(name[i] == '.' && j <= 8){
            j = 8;
        }else{
            s[j] = name[i];      //在这里写入文件名以及后缀
            if('a' <= s[j] && s[j] <= 'z')
                s[j] -= 0x20;
            j++;
        }
    }
    //寻找文件
    int x;
    for(x = 0; x < max;){
        if(finfo[x].name[0] == 0x00){
              break;
        }
        if ((finfo[x].type & 0x18) == 0) {
           for(int i = 0; i < 11; i++){
                if(finfo[x].name[i] != s[i]){
                    goto type_next_file;
                }
           }
           return finfo + x;;
        }
type_next_file:
        x++;
    }

    return 0;                    
}
