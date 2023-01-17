#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <unistd.h>

char *mapFile(char *path, size_t *sz, int *fd) {
    struct stat st;
    int rc = stat(path, &st);
    if (rc == -1) {
        perror("stat");
        exit(1);
    }

    int f = open(path, O_RDONLY);
    if (f == -1) {
        perror("open");
        exit(1);
    }

    void *map = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, f, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    *fd = f;
    *sz = st.st_size;

    return map;
}

uint64_t calcChecksum(char *data, size_t sz) {
    uint64_t checksum = 0;
    uint64_t *ptr = (uint64_t *)data;
    while ((void *)ptr < (void*)(data + sz)) {
        checksum += *ptr++;
    }
    char *cptr = (char *)ptr;
    while ((void *)cptr < (void*)(data + sz)) {
        checksum += *cptr;
    }
    return checksum;
}

int main(int argc, char *argv[]) {
    const char *xattr = "user.checksum";
    char *fn;
    bool reset_checksum;

    if (argc == 3 && strcmp(argv[1], "-r") == 0) {
        reset_checksum = true;
        fn = argv[2];
    } else if (argc == 2) {
        reset_checksum = false;
        fn = argv[1];
    } else {
        fprintf(stderr, "usage: %s [-r] <FILE>\n", argv[0]);
        return 1;
    }

    int fd;
    size_t sz;
    char *data = mapFile(fn, &sz, &fd);

    if (reset_checksum) {
        int rc = fremovexattr(fd, xattr);
        if (rc == -1 && errno != ENODATA) {
            perror("fremovexattr");
            return 1;
        }
        return 0;
    }

    const uint64_t crc = calcChecksum(data, sz);

    uint64_t prev_crc;
    if (fgetxattr(fd, xattr, &prev_crc, sizeof(prev_crc)) != sizeof(prev_crc)) {
        puts("prev_crc: NULL");
    } else {
        printf("prev_crc: %lx\n", prev_crc);
        if (crc != prev_crc) {
            fprintf(stderr, "checksum mismatch\n");
            return 1;
        }
        puts("checksum match!");
    }

    if (fsetxattr(fd, xattr, &crc, sizeof(crc), 0) == -1) {
        perror("fsetxattr");
        return 1;
    }

    return 0;
}
