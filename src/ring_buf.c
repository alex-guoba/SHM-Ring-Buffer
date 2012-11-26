

#include <sys/shm.h>
#include <unistd.h>

#include "shm_ctx.h"
#include "ring_buf.h"


uint32_t cacheline_size = 64;

static shm_data_t *shm = NULL;

shmctx_t    mutex_w;
shmctx_t    mutex_r;

int ringbuf_init(size_t size, size_t n, int key) {
    size_t initsize = 0;
    void *addr = NULL;
    char *ch;
    int id;

#ifdef _CACHE_ALIGN_
    csize = cpuinfo();
    if (csize > 0)
        cacheline_size = (uint32_t)csize;

    size = size_align(size, cacheline_size);
#endif

    initsize = sizeof(shm_data_t) + size * n;

    id = shmget(key, initsize, SHM_R|SHM_W|IPC_CREAT);
    if (id == 0)
        return -1;

    addr = shmat(id, NULL, 0);
    if (addr == (void*)-1) 
        return -2;

    shm = (shm_data_t*)addr;
    shm->size = size;
    shm->max = n;

    // init mutex_w
    ch = shm->channel;
    shmtx_init(&mutex_w, (shmtx_sh_t*)ch, 0x800);
    shmtx_init(&mutex_r, (shmtx_sh_t*)(ch + 64), 0x800);

    printf("index_r: %u, index_w: %u\n", shm->index_r, shm->index_w);

    return 0;
}


int ringbuf_push(const char *value) {
    static pid_t pid = 0;

    if (unlikely(pid == 0))
        pid = getpid();

    shmtx_lock(&mutex_w, pid);

    uint32_t index_w = shm->index_w;
    uint32_t next = NextPos(index_w, shm->max);
    if (next == shm->index_r) {
        shmtx_unlock(&mutex_w, pid);
        return Cache_Full;
    }

    memcpy(shm->datas + index_w * shm->size, value, shm->size);
    atomic_cmp_set(&shm->index_w, index_w, next);

    if (!shmtx_unlock(&mutex_w, pid)) {
        printf("push lock exception! %lu\n", *mutex_w.lock);
    }

    return 0;
}

int ringbuf_push_unlock(const char *value) {
    uint32_t next = NextPos(shm->index_w, shm->max);
    if (next == shm->index_r) {
        return Cache_Full;
    }

    memcpy(shm->datas + shm->index_w * shm->size, value, shm->size);
    shm->index_w = next;

    return 0;
}


/*
 *  使用回调的方式push，避免数据copy
 * */
int ringbuf_push_hook(CopyFunc func, const void *value) {
    static pid_t pid = 0;

    if (unlikely(pid == 0))
        pid = getpid();

    shmtx_lock(&mutex_w, pid);

    uint32_t index_w = shm->index_w;
    uint32_t next = NextPos(index_w, shm->max);
    if (next == shm->index_r) {
        shmtx_unlock(&mutex_w, pid);
        return Cache_Full;
    }

    //memcpy(shm->datas + index_w * shm->size, value, shm->size);
    if(func(shm->datas + index_w * shm->size, value, shm->size)) {
        shmtx_unlock(&mutex_w, pid);
        return Hook_Error;
    }
    atomic_cmp_set(&shm->index_w, index_w, next);

    if (!shmtx_unlock(&mutex_w, pid)) {
        printf("push lock exception! %lu\n", *mutex_w.lock);
    }

    return 0;
}

int ringbuf_push_hook_unlock(CopyFunc func, const void *value) {
    uint32_t next = NextPos(shm->index_w, shm->max);
    if (next == shm->index_r) {
        return Cache_Full;
    }

    if(func(shm->datas + shm->index_w * shm->size, value, shm->size)) {
        return Hook_Error;
    }
    shm->index_w = next;

    return 0;
}



int ringbuf_pop(char *value) {
    static pid_t pid = 0;

    if (unlikely(pid == 0))
        pid = getpid();

    shmtx_lock(&mutex_r, pid);
    uint32_t index_r = shm->index_r;
    if (shm->index_w == index_r) {
        shmtx_unlock(&mutex_r, pid);
        return No_Data;
    }

    uint32_t next = NextPos(index_r, shm->max);

    memcpy(value, shm->datas + index_r * shm->size, shm->size);
    atomic_cmp_set(&shm->index_r, index_r, next);

    if (!shmtx_unlock(&mutex_r, pid)) {
        printf("pop lock exception! %lu \n", *mutex_r.lock);
    }

    return 0;
}

int ringbuf_pop_unlock(char *value) {
    if (shm->index_w == shm->index_r) {
        return No_Data;
    }

    memcpy(value, shm->datas + shm->index_r * shm->size, shm->size);
    shm->index_r = NextPos(shm->index_r, shm->max);

    return 0;
}

void ringbuf_destory() {
    shmdt(shm);
    shm = NULL;
}

