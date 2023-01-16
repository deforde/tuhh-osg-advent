#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PAGE_SZ 4096

struct {
    alignas(PAGE_SZ) int strt;
    int cnt;
} mmap_data;

static void init_persistence(const char *path) {
    int f = open(path, O_RDWR);
    if (f == -1) {
        f = open(path, O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
        if (f == -1) {
            perror("open");
            exit(1);
        }
        if (ftruncate(f, sizeof(mmap_data)) == -1) {
            perror("ftruncate");
            exit(1);
        }
        write(f, &mmap_data, sizeof(mmap_data));
    }

    void *ret = mmap(&mmap_data, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, f, 0);
    if (ret == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    if (close(f) == -1) {
        perror("close");
        exit(1);
    }
}

static void deinit_persistence(void *addr) {
    if (munmap(addr, sizeof(int)) == -1) {
        perror("munmap");
        exit(1);
    }
}

int main(void) {
    init_persistence("mmap.persistent");

    printf("cnt = %i\n", mmap_data.cnt++);

    char cmd[256];
    snprintf(cmd, 256, "pmap %d", getpid());
    printf("---- system(\"%s\"):\n", cmd);
    system(cmd);

    deinit_persistence(&mmap_data);

    return 0;
}
