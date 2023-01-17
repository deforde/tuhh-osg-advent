#define _GNU_SOURCE
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/uio.h>
#include <unistd.h>

#define MAX_NLINES 128

int iovCompare(const void *a, const void *b) {
    struct iovec *aiov = (struct iovec*)a;
    struct iovec *biov = (struct iovec*)b;
    return strcmp(aiov->iov_base, biov->iov_base);
}

int main(void) {
    struct iovec iov[MAX_NLINES] = {0};
    size_t niov = 0;

    puts("Pre sorting:");
    for (;;) {
        char *line = NULL;
        size_t lsz = 0;
        ssize_t rc = getline(&line, &lsz, stdin);
        if (rc == -1) {
            if (errno != 0) {
                perror("getline");
                exit(1);
            }
            free(line);
            break;
        }
        iov[niov].iov_base = line;
        iov[niov].iov_len = strlen(line);
        niov++;
        printf("%s", line);
    }

    qsort(iov, niov, sizeof(*iov), iovCompare);

    puts("Post sorting:");
    if (writev(STDOUT_FILENO, iov, niov) == -1) {
        perror("writev");
        exit(1);
    }

    for (size_t i = 0; i < niov; i++) {
        free(iov[i].iov_base);
    }
}
