
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "ring_buf.h"
#include "cpu_timer.h"

#define SLEEP(x) usleep(x)

typedef void (*cycle)(void);

typedef struct {
    int nRead;
    int nWrite;
} CONFIG;

typedef struct {
    int nValue;
    char szValue[32];
}Node;

#define Max_Node    10000
#define Node_Shm_Key    0x83828988
#define Usleep_Time 1000

void fork_childs(int n, cycle proc) {
    pid_t pid;
    int i = 0;
    for (i = 0; i < n; i++) {
        pid = fork();
        if (pid < 0) {
            printf("fork error!\n");
        }
        else if (pid == 0) {
            proc();
            printf("funtion over!\n");
            exit(0);
        }
        printf("PID %d start, %p\n", pid, proc);
    }
}

void read_cycle(void) {
    unsigned int cnt = 0;
    unsigned int errcnt = 0;
    Node node;
    int ret;
    int ic = 0;
 
    if (ringbuf_init(sizeof(Node), Max_Node, Node_Shm_Key) ) {
        printf("init error!\n");
        return;
    }

    while(1) {
        node.nValue = (int)cnt;
        ret = ringbuf_pop((char*)&node);
        if (ret) {
            errcnt++;
            //printf("pop err: %d, cnt:%u\n", ret, cnt);
            SLEEP(Usleep_Time);
            ic++;
            if (ic == 2000)
                printf("\tpop err: %d, cnt:%u\n", ret, cnt);
            continue;
        }
        ic = 0;
        cnt++;
        if ((cnt & 0xFFFFF) == 0)
            printf("\tpop cnt: %u, errcnt: %u\n", cnt, errcnt);
    }
}

void write_cycle(void) {
    Node node;
    int ret;
    unsigned int cnt = 0;
    unsigned int errcnt = 0;

    if (ringbuf_init(sizeof(Node), Max_Node, Node_Shm_Key) ) {
        printf("init failed! \n");
        return;
    }

    CPU_TIMER_DEFINE(1);
    CPU_TIMER_INIT(1);

    while(1) {
        ret = ringbuf_push((char*)&node, sizeof(node));
        if (ret) {
            errcnt++;
            //printf("push ret: %d, cnt: %u\n", ret, cnt);
            SLEEP(Usleep_Time);
            continue;
        }
        cnt++;
        if (cnt == 0xFFFFFF) {
            printf("push total: %u, errcnt: %u\n", cnt, errcnt);
            break;
        }
    }
    CPU_TIMER_PRINT(1, "push ");
}


int main(int argc, char *argv[]) {
    CONFIG cfg;

    cfg.nRead = 4;
    cfg.nWrite = 4;

    fork_childs(cfg.nRead, read_cycle);
    fork_childs(cfg.nWrite, write_cycle);

    getchar();

    return 0;
}
