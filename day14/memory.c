#include "memory.h"
#include "asmfunc.h"

/**
 *
 *弄清这个函数的具体细节
 */


//计算机中的地址都为无符号数

unsigned int memtest(unsigned int start, unsigned int end){
    char flg486 = 0;            //检测是否为486的标志
    unsigned int eflg, cr0;     //通过CR0寄存器的30位和29位来控制cache是否开启（具体见osdev：CPU Registers x86部分）

    eflg = io_load_eflags();
    eflg |= EFLAGS_AC_BIT;       //设置EFLAGS第18位
    io_store_eflags(eflg);
    eflg = io_load_eflags();
    
    //之前设置了该寄存器的的标志，如果cpu为i386(i386的第18位不能被修改)，则自动变回0
    if((eflg & EFLAGS_AC_BIT)){
        flg486 = 1;
    }

    //如果为386以上的机器，则修改cpu中EFLAGS的标志，需要改回
    //此处迷惑
    eflg &= ~EFLAGS_AC_BIT;     //这里为何将这个EFLAG的标志位都设位0？
    io_store_eflags(eflg);
    
    //在检测内存空间时禁止cache，避免在检测时cpu读写的不是内存而是cache
    if(flg486){
        cr0 = io_load_cr0();
        cr0 |= CR0_CACHE_DISABLE;
        io_store_cr0(cr0);
    }
    
    //检测内存
    unsigned int ret = memtest_sub(start, end);

    //检测完毕后开启cache
    if(flg486){
        cr0 = io_load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE;
        io_store_cr0(cr0);
    }

    return ret;
}

/*
 *内存检测部分的代码有多个版本，其分别为了解决不同的问题而开发的。
 *这里为保证系统的健壮性和高效性，使用汇编版本的按0x1000大小检测的版本
 *这个也给出与汇编版含义相同的c实现（为避免编译器优化问题，故采用汇编版）
 *另一个按4字节地址检测的版本可参考书籍
 * **/

//非汇编版本的按0x1000大小检查
/*
unsigned int memtest_sub(unsigned int start, unsigned int end){
    unsigned int i, *p, old;
    unsigned int pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;  //只是用于检测内存是否可写，这两个值无其它含义

    for(i = start; i <= end; i+=0x1000){
        //这里加0xffc的含义是只检测每一页的最后四个字节
        
        // 为何这里只检查最后四个地址？
        // 内存页的最后一个地址（如 0xXXXXXFFC）通常是最可能出错的边界（例如未映射区域或硬件保留区）。
        // 如果连最后一字节都能正确读写，整页大概率有效。说明该页被正常映射
          
        p = (unsigned int *)(i + 0xffc);

        //以下程序的主要目的就是为了检测内存的读写是否正常，通过写入取反一系列操作的方法来检测
        old = *p;
        *p = pat0;
        *p ^= 0xffffffff;

        if(*p != pat1){
        not_memory:
            *p = old;
            break;
        }

        *p ^= 0xffffffff;       //如果正常的话，两次异或运算的结果为最初的值
        if(*p != pat0){
            goto not_memory;
        }
        *p = old;
    }

    return i;
}
*/

//内存管理部分的代码
void memman_init(struct MemMan *man) {
  man->frees = 0;    // 可用信息数目
  man->maxfrees = 0; // 用于观察可用状况：frees的最大值
  man->lostsize = 0; // 释放失败的内存的大小总和
  man->losts = 0;    // 释放失败次数
}

unsigned int memman_total(struct MemMan *man){
    unsigned int i, total;

    for(i = 0; i < man->frees; i++){
        total += man->free[i].size;
    }
    
    return total;
}

//分配
unsigned int memman_alloc(struct MemMan *man, unsigned int size){
    for(unsigned int i = 0; i < man->frees; i++){
        if(man->free[i].size >= size){
            //找到适合大小的块并进行分配
            unsigned int addr = man->free[i].addr;
            man->free[i].addr += size;
            man->free[i].size -= size;

            if(man->free[i].size == 0){
            //如果一个块的可利用空节为0，从表中删除这个页项
                man->frees--;
                //这里使用数组移动方式来删除一项（由此可以对其进行改进，使用链表结构来作为）
                for(; i < man->frees; i++){
                    man->free[i] = man->free[i+1];
                }
            }

            return addr;
        }
    }
    return 0;
}

int memman_free(struct MemMan *man, unsigned int addr, unsigned int size){
    int i, j;

    //先确定插入的位置
    for(i = 0; i < man->frees; i++){
        if(man->free[i].addr > addr)
            break;
    }
    
    // free[i - 1].addr < addr < free[i].addr
    if(i > 0){
        //前部有可合并的内存
        if(man->free[i - 1].addr + man->free[i - 1].size == addr){
            man->free[i - 1].size += size;
            if(i < man->frees){
                //后部也有可合并内存
                if(addr + size == man->free[i].addr){
                    man->free[i - 1].size += man->free[i].size;
                    man->frees--;
                    //合并完成后整合记录
                    for(; i < man->frees; i++){
                        man->free[i] = man->free[i + 1];
                    }
                }
            }
        }
    }
    
    //前部不能合并但后部可以合并
    if(i < man->frees){
        if(addr + size == man->free[i].addr){
            man->free[i].addr = addr;
            man->free[i].size += size;

            return 0;
        }
    }

    //两端都不能移动
    if(man->frees < MEMMAN_FREES){
        //如果两端都不能合并时，从i向后移动空出一个存储空间
        for(j = man->frees; j > i; j--){
            man->free[j] = man->free[j-1];
        }
        man->frees++;

        //更新最大值
        if(man->maxfrees < man->frees)
            man->maxfrees = man->frees;

        man->free[i].addr = addr;
        man->free[i].size = size;
        
        return 0;
    }

    //如果回收失败，更新结构体相关的数据
    man->losts++;
    man->lostsize += size;

    return -1;
}

//采用以页为单位的分配和回收内存
unsigned int memman_alloc_4K(struct MemMan *man, unsigned int size){
    //这里采用一种巧妙的方法来地址向上舍入(即不满足一个页的数据也使用一页来存储)
    size = (size + 0xfff) & 0xfffff000;
    return memman_alloc(man, size);
}

int memman_free_4K(struct MemMan *man, unsigned int addr, unsigned int size) {
  size = (size + 0xfff) & 0xfffff000;
  return memman_free(man, addr, size);
}
