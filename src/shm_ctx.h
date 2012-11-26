
#ifndef _SHM_CTX_H_
#define _SHM_CTX_H_

#include <stdint.h>

#include "gcc_atomic.h"
#include "cpu_timer.h"

typedef unsigned long atomic_uint_t;
typedef volatile atomic_uint_t atomic_t;

typedef uintptr_t       uint_t;

typedef struct {
   atomic_t lock;
} shmtx_sh_t;

typedef struct {
    atomic_t *lock;
    unsigned int spin;
} shmctx_t;

uint_t shmtx_force_unlock(shmctx_t *mtx);

void shmtx_init(shmctx_t *mtx, shmtx_sh_t *addr, unsigned int spin) {
    mtx->lock = &addr->lock;
    mtx->spin = spin;
}

uint_t shmctx_trylock(shmctx_t *mtx, uint_t id) {
    return (*mtx->lock == 0 && atomic_cmp_set(mtx->lock, 0, id));
}

void shmtx_lock(shmctx_t *mtx, uint_t id) {
    int i, n;
    static unsigned long long ticks_per_second = 0;
    unsigned long long ticks_nop = 0;

    if (unlikely(ticks_per_second == 0)) {
        ticks_per_second = GetCpuFreq();
    }

    for ( ;; ) {
        if (*mtx->lock == 0 && atomic_cmp_set(mtx->lock, 0, id)) {
            return;
        }

        for (n = 1; n < mtx->spin; n <<= 1) {
            for (i = 0; i < n; i++) {
                cpu_pause();
                ticks_nop++;
            }

            if (*mtx->lock == 0 && atomic_cmp_set(mtx->lock, 0, id)) {
                return;
            }
        }

        // 防死锁
        if (++ticks_nop > ticks_per_second/50) {
            printf("ticks_nop: %llu, %lu\n", ticks_nop, *mtx->lock);
            shmtx_force_unlock(mtx);
            ticks_nop = 0;
        }

        relinquish();
    }
}


uint_t shmtx_unlock(shmctx_t *mtx, uint_t id) {
    return atomic_cmp_set(mtx->lock, id, 0);
}

uint_t shmtx_force_unlock(shmctx_t *mtx) {
    return atomic_test_and_set(mtx->lock, 0);
}

#endif

