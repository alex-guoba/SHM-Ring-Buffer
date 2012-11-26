/*
 *  实现一个基于自旋锁的环形队列
 *  考虑到多进程可以同时写，自旋锁的value基于共享内存
 *  同时需要考虑进程被kill时造成的死锁
 * */

#ifndef _RING_BUF_2_H_
#define _RING_BUF_2_H_

#include <stdio.h>
#include <stdint.h>

#define Channel_Size 256

#define size_align(d, a)     (((d) + (a - 1)) & ~(a - 1))

#define Next_Slot(x, max) ((x) + 1) % (max)

typedef int (*CopyFunc)(void *dest, const void *src, size_t n);

typedef enum {
    Cache_Full = 1,
    No_Data = 2,
    Hook_Error = 3,
} Statue;

typedef struct {
    char channel[Channel_Size];

    size_t size;
    size_t max;

    //char resv[128 - sizeof(atomic_t) - 2*sizeof(size_t) - 2*sizeof(uint32_t)];

    volatile uint32_t index_r;
    volatile uint32_t index_w;     // free node tail

    char pending[60];
    char datas[0];
} shm_data_t;

#define HEAD_SIZE sizeof(struct shm_data_t)

int ringbuf_init(size_t size, size_t n, int key);

int ringbuf_push(const char *value, size_t len);
int ringbuf_push_unlock(const char *value, size_t len);

int ringbuf_push_hook(CopyFunc func, const void *value, size_t len);
int ringbuf_push_hook_unlock(CopyFunc func, const void *value, size_t len);

int ringbuf_pop(char *value);
int ringbuf_pop_unlock(char *value);

void ringbuf_destory();

#endif

