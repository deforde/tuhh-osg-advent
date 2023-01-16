#include <errno.h>
#include <stdalign.h>
#include <stdio.h>

#include <sys/inotify.h>
#include <unistd.h>

int main(void) {
    int fd = inotify_init();
    if (fd == -1) {
        perror("inotify_init");
        return 1;
    }

    int wd = inotify_add_watch(fd, ".", IN_OPEN | IN_CLOSE | IN_ACCESS);
    if (wd == -1) {
        perror("inotify_add_watch");
        return 1;
    }

    alignas(alignof(struct inotify_event)) char buf[4096] = {0};

    for (;;) {
        ssize_t rsz = read(fd, buf, sizeof(buf));
        if (rsz == -1) {
            if (errno == EAGAIN) {
                continue;
            }
            perror("read");
            return 1;
        }
        if (rsz == 0) {
            break;
        }

        struct inotify_event *event = (struct inotify_event *)buf;
        if (event->len) {
            printf("%s ", event->name);
        }

        if (event->mask & IN_OPEN) {
            printf("IN_OPEN");
        }
        else if (event->mask & IN_ACCESS) {
            printf("IN_ACCESS");
        }
        else if (event->mask & IN_CLOSE) {
            printf("IN_CLOSE");
        }

        puts("");
    }

    close(fd);

    return 0;
}
