#define _GNU_SOURCE
#include <errno.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <linux/futex.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

static int futex(atomic_int *addr, int op, uint32_t val) {
    return syscall(SYS_futex, addr, op, val, NULL, NULL, 0);
}

static void ftxWait(atomic_int *ftx, uint32_t val) {
    for (;;) {
        int s = futex(ftx, FUTEX_WAIT, val);
        if (s == -1) {
            if (errno == EAGAIN) {
                continue;
            }
            perror("futex-FUTEX_WAIT");
            exit(1);
        }
        break;
    }
}

static void ftxWake(atomic_int *ftx) {
    if (futex(ftx, FUTEX_WAKE, 1) == -1) {
        perror("futex-FUTEX_WAKE");
        exit(1);
    }
}

static void semInit(atomic_int *sem, int init_val) {
    atomic_init(sem, init_val);
}

static void semDecr(atomic_int *sem) {
    for (;;) {
        int val = atomic_load(sem);
        if (val > 0) {
            if (atomic_compare_exchange_strong(sem, &val, val - 1)) {
                break;
            }
        } else {
            ftxWait(sem, 0);
        }
    }
}

static void semIncr(atomic_int *sem) {
    int prev = atomic_fetch_add(sem, 1);
    if (prev == 0) {
        ftxWake(sem);
    }
}

#define BB_CAP 3

typedef struct {
    atomic_int slots;
    atomic_int elems;

    atomic_int lock;

    size_t rd_idx;
    size_t wr_idx;

    void *store[BB_CAP];
} bb_t;

static void bbInit(bb_t *bb) {
    semInit(&bb->slots, BB_CAP);
    semInit(&bb->elems, 0);

    semInit(&bb->lock, 1);

    bb->rd_idx = 0;
    bb->wr_idx = 0;
    memset(bb->store, 0, sizeof(bb->store));
}

static void *bbPop(bb_t *bb) {
    void *ret = NULL;

    semDecr(&bb->elems);

    semDecr(&bb->lock);
    ret = bb->store[bb->rd_idx];
    bb->rd_idx = (bb->rd_idx + 1) % BB_CAP;
    semIncr(&bb->lock);

    semIncr(&bb->slots);

    return ret;
}

static void bbPush(bb_t *bb, void *data) {
    semDecr(&bb->slots);

    semDecr(&bb->lock);
    bb->store[bb->wr_idx] = data;
    bb->wr_idx = (bb->wr_idx + 1) % BB_CAP;
    semIncr(&bb->lock);

    semIncr(&bb->elems);
}

static void childFunc(bb_t *bb, atomic_int *sem) {
    char *data[] = {
        "hello", "world", "from", "the", "child", "process", "!"
    };
    sleep(1);
    bbInit(bb);
    puts("sending bb initialisation signal");
    semIncr(sem);
    for (size_t i = 0; i < sizeof(data) / sizeof(*data); i++) {
        bbPush(bb, data[i]);
        if (i > 2) {
            sleep(1);
        }
    }
    bbPush(bb, NULL);
}

static void parentFunc(bb_t *bb, atomic_int *sem) {
    semDecr(sem);
    puts("bb initialisation signal received");
    char *elem;
    do {
        elem = bbPop(bb);
        printf("parent: %p = '%s'\n", elem, elem);
    } while (elem);
}

int main(void) {
    char *shrd_mem = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    atomic_int *sem = (atomic_int*)&shrd_mem[0];
    bb_t *bb = (bb_t*)&shrd_mem[sizeof(bb_t)];

    semInit(sem, 0);

    int chld = fork();
    if (chld == -1) {
        perror("error");
        return 1;
    }
    if (chld == 0) {
        sleep(1);
        childFunc(bb, sem);
        return 0;
    }
    parentFunc(bb, sem);

    return 0;
}
