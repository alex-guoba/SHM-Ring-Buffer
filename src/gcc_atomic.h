
#ifndef _G2C_ATOMIC_H_
#define _G2C_ATOMIC_H_

#include <sched.h>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)


#define atomic_cmp_set(lock, old, set)  \
    __sync_bool_compare_and_swap(lock, old, set)

#define atomic_fetch_add(value, add)   \
    __sync_fetch_and_add(value, add)

#define memory_barrier()  \
    __sync_synchronize()

#define cpu_pause()   \
    __asm__ ("pause")

#define atomic_test_and_set(lock, value)  \
    __sync_lock_test_and_set(lock, value)

#define relinquish()   \
    sched_yield()

#endif

