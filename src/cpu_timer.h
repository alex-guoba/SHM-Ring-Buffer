#ifndef __CPU_TIMER_H
#define __CPU_TIMER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>


#define CPU_TIMER_DEFINE(t) __typeof(0LL) __cputimer##t
#define CPU_TIMER_INIT(t) __cputimer##t = rdtsc()
#define CPU_TIMER_PRINT(t, msg) Elapsedcount(rdtsc()-__cputimer##t, GetCpuFreq(), "->"#msg)


static inline uint64_t
rdtsc()
{
    unsigned int lo, hi;
#ifdef __linux__
    /* We cannot use "=A", since this would use %rax on x86_64 */
    __asm__ __volatile__ (
            "rdtsc"
            : "=a" (lo), "=d" (hi)
            );
#else
#error "not done yet "
#endif
    return (uint64_t)hi << 32 | lo;
}



static inline int
Sleep(unsigned int nMilliseconds)
{   
    struct timespec ts;
    ts.tv_sec = nMilliseconds / 1000;
    ts.tv_nsec = (nMilliseconds % 1000) * 1000000;

    return nanosleep(&ts, NULL);
}

static int
SplitString(char* pString, char c, char* pElements[], int n)
{   
    char *p1, *p2;
    int i = 0;

    for (p1=p2=pString; *p1; p1++) {
        if (*p1 == c) {
            *p1 = 0;
            if (i < n)
                pElements[i++] = p2;
            p2 = p1+1;
        }
    }
    if (i < n)
        pElements[i++] = p2;
    return i;
}

static uint64_t
CalcCpuFreq()
{
    uint64_t t1;
    uint64_t t2;

    t1 = rdtsc();
    Sleep(100);
    t2 = rdtsc();
    return (t2-t1)*10;
}

static uint64_t GetCpuFreq()
{   
    static uint64_t freq = 0;
    char buf[1024], *p[2];

    if (0 != freq) {
        return freq;
    }

    FILE* fp = fopen("/proc/cpuinfo", "rb");
    if (fp != NULL) {
        while (fgets(buf, sizeof(buf), fp) != NULL) {
            if (2==SplitString(buf, ':', p, 2) && 0==strncasecmp(p[0], "cpu MHz", 7)) {
                double f = strtod(p[1], NULL);
                freq = (uint64_t)(f * 1000000.0);
                /*printf("p[1]=%s f=%f freq=%llu\n", p[1], f,freq);*/
                break;
            }
        }
        fclose(fp);
    }
    if (0 == freq) {
        freq = CalcCpuFreq();
    }
    return freq;
}

static inline void Elapsed(uint64_t c, uint64_t cpu_freq){
    printf("time second : %lld \n", c/cpu_freq);
}

static inline void Elapsedms(uint64_t c, uint64_t cpu_freq){
    printf("time ms : %f \n", (double)c/cpu_freq * 1000.0);
}

static inline void Elapsedus(uint64_t c, uint64_t cpu_freq){
    printf("time us : %f \n", ((double)c/cpu_freq) * 1000000.0);
}

static inline void Elapsedcount(uint64_t c, uint64_t cpu_freq, char* msg){
    printf("%s time count : %lld \n", msg, c);
}

#endif
