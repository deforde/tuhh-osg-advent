#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fcntl.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static char chld_stk[4096];
static int data[] = { 0, 1, 2 };

static void printData(void) {
    printf("\tdata = [ ");
    for (size_t i = 0; i < sizeof(data) / sizeof(*data); i++) {
        printf("%i%s", data[i], (i < sizeof(data) / sizeof(*data) - 1) ? ", " : "");
    }
    printf(" ]\n");
}

static int childFunc(void *p) {
    printf("\tchild:\n");
    printf("\t\tppid: %i\n", getpid());
    printf("\t\tpid: %i\n", getpid());
    printf("\t\ttid: %i\n", gettid());
    printf("\t\tuid: %i\n", getuid());
    for (size_t i = 0; i < sizeof(data) / sizeof(*data); i++) {
        data[i]++;
    }
    printData();

    if (p) {
        int f = open("/proc/self/uid_map", O_RDWR);
        if (f == -1) {
            perror("open");
            exit(1);
        }

        const ssize_t wsz = write(f, p, strlen(p));
        if (wsz == -1) {
            perror("write");
            exit(1);
        }

        if (close(f) == -1) {
            perror("close");
            exit(1);
        }

        setuid(0);
        printf("\t\tnew uid: %i\n", getuid());
    }

    return 0;
}

int main() {
    {
        puts("clone as fork:");
        printf("\tparent:\n");
        printf("\t\tppid: %i\n", getpid());
        printf("\t\tpid: %i\n", getpid());
        printf("\t\ttid: %i\n", gettid());
        printf("\t\tuid: %i\n", getuid());

        int pid = clone(childFunc, &chld_stk[sizeof(chld_stk) - 1], SIGCHLD, NULL);
        if (pid == -1) {
            perror("clone");
            return 1;
        }

        puts("\twaiting for child process to terminate...");
        int wstatus;
        if (waitpid(pid, &wstatus, 0) == -1) {
            perror("waitpid");
            return 1;
        }
        puts("\tchild process terminated");
        printData();
        puts("");
    }

    {
        puts("clone sharing address space:");
        printf("\tparent:\n");
        printf("\t\tppid: %i\n", getpid());
        printf("\t\tpid: %i\n", getpid());
        printf("\t\ttid: %i\n", gettid());
        printf("\t\tuid: %i\n", getuid());

        int pid = clone(childFunc, &chld_stk[sizeof(chld_stk) - 1], SIGCHLD | CLONE_VM, NULL);
        if (pid == -1) {
            perror("clone");
            return 1;
        }

        puts("\twaiting for child process to terminate...");
        int wstatus;
        if (waitpid(pid, &wstatus, 0) == -1) {
            perror("waitpid");
            return 1;
        }
        puts("\tchild process terminated");
        printData();
        puts("");
    }

    {
        puts("clone as process thread:");
        printf("\tparent:\n");
        printf("\t\tppid: %i\n", getpid());
        printf("\t\tpid: %i\n", getpid());
        printf("\t\ttid: %i\n", gettid());
        printf("\t\tuid: %i\n", getuid());

        int pid = clone(childFunc, &chld_stk[sizeof(chld_stk) - 1], CLONE_VM | CLONE_THREAD | CLONE_SIGHAND, NULL);
        if (pid == -1) {
            perror("clone");
            return 1;
        }

        // puts("\tsleeping to allow child thread to terminate...");
        sleep(1);
        printData();
        puts("");
    }

    {
        puts("clone as fork, but with a new user namespace:");
        printf("\tparent:\n");
        printf("\t\tppid: %i\n", getpid());
        printf("\t\tpid: %i\n", getpid());
        printf("\t\ttid: %i\n", gettid());
        printf("\t\tuid: %i\n", getuid());

        char *arg = calloc(1, 128);
        snprintf(arg, 127, "0 %d 1\n", getuid());
        int pid = clone(childFunc, &chld_stk[sizeof(chld_stk) - 1], SIGCHLD | CLONE_NEWUSER, arg);
        if (pid == -1) {
            perror("clone");
            return 1;
        }

        puts("\twaiting for child process to terminate...");
        int wstatus;
        if (waitpid(pid, &wstatus, 0) == -1) {
            perror("waitpid");
            return 1;
        }
        puts("\tchild process terminated");

        printData();
        puts("");
    }

    return 0;
}
