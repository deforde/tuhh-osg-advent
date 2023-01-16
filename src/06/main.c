#define _GNU_SOURCE
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sys/mman.h>

#define PAGE_SZ 4096

static int do_exit = 0;

void sigAction(int sig, siginfo_t *info, void *ucontext) {
    if (sig == SIGSEGV) {
        puts("handling SIGSEGV");
        uintptr_t addr = (uintptr_t)info->si_addr & (~(PAGE_SZ - 1));
        void *ret = mmap((void*)addr, PAGE_SZ, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (ret == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }
    } else if (sig == SIGILL) {
        puts("handling SIGILL");
        ucontext_t *ctx = (ucontext_t*)ucontext;
        ctx->uc_mcontext.gregs[REG_RIP] += 4;
    } else if (sig == SIGINT) {
        puts("handling SIGINT");
        do_exit = 1;
    } else {
        printf("received unexpected signal: %i\n", sig);
        exit(1);
    }
}

struct sigaction sa = {
    .sa_sigaction = sigAction,
    .sa_flags = SA_SIGINFO | SA_RESTART,
};

int main(void) {
    int sigs[] = {SIGSEGV, SIGILL, SIGINT};
    for (size_t i = 0; i < sizeof(sigs) / sizeof(*sigs); i++) {
        if (sigaction(sigs[i], &sa, NULL) == -1) {
            perror("sigaction");
            return 1;
        }
    }

    int *ptr = (int*)0xdeadbeef;
    *ptr = 1;

    asm("ud2; ud2;");

    puts("waiting for SIGINT...");
    while (!do_exit) {
        sleep(1);
    }
    puts("exiting");

    return 0;
}
