#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <time.h>

#define INOTIFY_EVENT struct inotify_event

char *FILES[2] = {"toWatch.txt", "file1.txt"};

int main()
{
    int inotify = inotify_init();

    if (inotify == -1)
    {
        perror("Error starting inotify");
        return -1;
    }

    int fd = inotify_add_watch(inotify, ".", IN_MODIFY | IN_CLOSE_WRITE);
    if (fd == -1)
    {
        fprintf(stdout, "Failed adding watch to file %s\n", ".");
    }

    char *buffer = malloc(4096);
    INOTIFY_EVENT *event;
    time_t lastReload = time(0);

    while (1)
    {
        ssize_t bytesRead = read(inotify, buffer, 4096);
        printf("Bytes read %ld\n", bytesRead);

        for (char *ptr = buffer; ptr <= buffer + bytesRead; ptr += sizeof(INOTIFY_EVENT) + event->len)
        {
            event = (INOTIFY_EVENT *)ptr;
            time_t eventTime = time(0);
            printf("Time:%ld\n", eventTime);
            if (event->mask & IN_CLOSE_WRITE || event->mask & IN_MODIFY)
            {
                printf("File Changed: %s\n", event->name);
                printf("Event Length: %d\n", event->len);
                if (eventTime - lastReload >= 2)
                {
                    printf("Time to reload!");
                    lastReload = eventTime;
                }
            }
        }
    }
    return 0;
}
