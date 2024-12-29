#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define INOTIFY_EVENT struct inotify_event
#define MAX_PATH_SIZE 100

void executeBuildScript(char *script)
{
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {   
        if (execlp("/bin/sh" , "/bin/sh",script, NULL) == -1)
        {
            perror("execlp");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
    }
}

int checkIfBuildScriptExists(char* script) {
    if (script != NULL)
    {
        FILE *scriptFD = fopen(script, "r");
        if (scriptFD == NULL)
        {
            return -1;
        }
        fclose(scriptFD);
    }
    return 0;
}


int main(int argc, char *argv[])
{

    char *build_script = NULL;
    char *watchFile[100];
    int watchFileCount = 0;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-bs") == 0 && argc > i + 1)
        {
            build_script = argv[i + 1];
            i++;
        }
        if (strcmp(argv[i], "-watch") == 0 && argc > i + 1)
        {
            watchFile[watchFileCount++] = argv[i + 1];
            i++;
        }
    }

    if (checkIfBuildScriptExists(build_script) != 0) {
        printf("Script file does not exist\n");
        return -1;
    }

    int inotify = inotify_init();

    if (inotify == -1)
    {
        perror("Error starting inotify");
        return -1;
    }

    for (int i = 0; i < watchFileCount;i++) {
        int fd = inotify_add_watch(inotify, watchFile[i], IN_MODIFY | IN_CLOSE_WRITE);
        if (fd == -1)
        {
            fprintf(stdout, "Failed adding watch to file %s\n", watchFile[i]);
            return -1;
        }
    }


    char *buffer = malloc(4096);
    INOTIFY_EVENT *event;
    if (build_script != NULL) executeBuildScript(build_script);
    time_t lastReload = time(0);

    while (1)
    {
        ssize_t bytesRead = read(inotify, buffer, 4096);

        for (char *ptr = buffer; ptr <= buffer + bytesRead; ptr += sizeof(INOTIFY_EVENT) + event->len)
        {
            event = (INOTIFY_EVENT *)ptr;
            time_t eventTime = time(0);
            if (event->mask & IN_CLOSE_WRITE || event->mask & IN_MODIFY)
            {
                if (eventTime - lastReload >= 2)
                {
                    executeBuildScript(build_script);
                    lastReload = eventTime;
                }
            }
        }
    }
    return 0;
}
