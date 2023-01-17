#define _GNU_SOURCE
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <pthread.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
    char *cmd;
    pid_t pid;
    int pstdin;
    int pstdout;
} proc_t;

static size_t nprocs = 0;
static proc_t *procs = NULL;

static void *stdinReadThread(__attribute__((unused)) void *p) {
    char buf[4096] = {0};

    for (;;) {
        ssize_t rsz = read(STDIN_FILENO, buf, sizeof(buf));
        if (rsz == -1) {
            perror("read");
            exit(1);
        }
        if (rsz == 0) {
            for (size_t i = 0; i < nprocs; i++) {
                if (procs[i].pid != 0) {
                    close(procs[i].pstdin);
                }
            }
            return NULL;
        }
        for (size_t i = 0; i < nprocs; i++) {
            if (procs[i].pid != 0) {
                write(procs[i].pstdin, buf, rsz);
            }
        }
    }
}

static void procRead(proc_t *proc, char *data, size_t sz) {
    const ssize_t rsz = read(proc->pstdout, data, sz);
    if (rsz == -1) {
        perror("read");
        exit(1);
    }
    if (rsz == 0) {
        int wstatus;
        if (waitpid(proc->pid, &wstatus, 0) == -1) {
            perror("waitpid");
            exit(1);
        }
        printf("[%s] child process exited with code: %i\n", proc->cmd, WEXITSTATUS(wstatus));
        proc->pid = 0;
        return;
    }
    char prev = 0;
    for (ssize_t i = 0; i < rsz; i++) {
        if (prev == '\n' || i == 0) {
            printf("[%s] ", proc->cmd);
        }
        putchar(data[i]);
        prev = data[i];
    }
}

int main(int argc, char *argv[]) {
    nprocs = argc - 1;
    procs = calloc(1, nprocs * sizeof(proc_t));

    for (size_t i = 1; i < (size_t)argc; i++) {
        char *arg = argv[i];
        const size_t proc_idx = i - 1;
        procs[proc_idx].cmd = strdup(arg);

        size_t nargs = 1;
        for (size_t j = 0; j < strlen(arg); j++) {
            if (arg[j] == ' ') {
                nargs++;
            }
        }

        char *spwn_cmd = NULL;
        char **spwn_args = calloc(1, (nargs + 1) * sizeof(char*));
        size_t spwn_arg_idx = 0;

        char *tok = strtok(arg, " ");
        while(tok) {
            if (spwn_cmd == NULL) {
                spwn_cmd = tok;
            }

            spwn_args[spwn_arg_idx++] = tok;

            tok = strtok(NULL, " ");
        }

        int pstdin[2];
        int pstdout[2];
        if (pipe2(pstdin, O_CLOEXEC)) {
            return 1;
        }
        if (pipe2(pstdout, O_CLOEXEC)) {
            return 1;
        }

        posix_spawn_file_actions_t fa;
        posix_spawn_file_actions_init(&fa);
        posix_spawn_file_actions_adddup2(&fa, pstdin[0], STDIN_FILENO);
        posix_spawn_file_actions_adddup2(&fa, pstdout[1], STDOUT_FILENO);

        posix_spawnattr_t attr;
        posix_spawnattr_init(&attr);

        pid_t pid;
        int s = posix_spawnp(&pid, spwn_cmd, &fa, &attr, spwn_args, environ);
        if (s != 0) {
            errno = s;
            perror("posix_spawn");
            return 1;
        }

        procs[proc_idx].pid = pid;
        procs[proc_idx].pstdin = pstdin[1];
        close(pstdin[0]);
        procs[proc_idx].pstdout = pstdout[0];
        close(pstdout[1]);

        printf("[%s] started filter as pid %i\n", procs[proc_idx].cmd, pid);

        posix_spawn_file_actions_destroy(&fa);
        free(spwn_args);
    }

    pthread_t stdin_read_thread;
    if (pthread_create(&stdin_read_thread, NULL, stdinReadThread, NULL) == -1) {
        perror("pthread_create");
        exit(1);
    }

    for (;;) {
        int nfds = 0;
        fd_set readfds;
        FD_ZERO(&readfds);
        for (size_t i = 0; i < nprocs; i++) {
            if (procs[i].pid != 0) {
                FD_SET(procs[i].pstdout, &readfds);
                if (nfds < procs[i].pstdout) {
                    nfds = procs[i].pstdout;
                }
            }
        }

        if (nfds == 0) {
            break;
        }

        int rc = select(nfds + 1, &readfds, NULL, NULL, NULL);
        if (rc == -1) {
            perror("select");
            exit(1);
        }

        static char buf[4096] = {0};
        for (size_t i = 0; i < nprocs; i++) {
            if (FD_ISSET(procs[i].pstdout, &readfds)) {
                procRead(&procs[i], buf, sizeof(buf));
            }
        }
    }

    for (size_t i = 0; i < nprocs; i++) {
        free(procs[i].cmd);
    }
    free(procs);

    return 0;
}
