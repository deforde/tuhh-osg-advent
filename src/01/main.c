#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static char buf[4096] = {0};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        char * pathname = argv[i];
        int f = open(pathname, O_RDONLY);
        if (f == -1) {
            perror("open");
            exit(1);
        }

        ssize_t rsz;
        do {
            rsz = read(f, buf, sizeof(buf));
            if (rsz == -1) {
                perror("read");
                exit(1);
            }
            fwrite(buf, rsz, 1, stdout);
        } while (!rsz);

        if (close(f) == -1) {
            perror("read");
            exit(1);
        }
    }

    exit(0);
}
