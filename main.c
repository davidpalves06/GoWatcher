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
    printf("%s\n",script);
    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        if (execlp(script, script, NULL) == -1)
        {
            perror("execlp");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
        {
            printf("Command exited with status %d\n", WEXITSTATUS(status));
        }
    }
}

int getFullPathForScript(char* script,char* fullScriptPath) {
    char cwd[MAX_PATH_SIZE];

    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        snprintf(fullScriptPath, sizeof(cwd) + sizeof(script), "%s/%s", cwd, script);
        return 1;
    } else {
        perror("getcwd");
        return -1;
    }
}

int main(int argc, char *argv[])
{

    char *build_script = NULL;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-bs") == 0 && argc > i + 1)
        {
            build_script = argv[i + 1];
            i++;
        }
    }

    if (build_script != NULL)
    {
        FILE *scriptFD = fopen(build_script, "r");
        if (scriptFD == NULL)
        {
            fprintf(stderr, "Script file does not exist. %d\n", errno);
            fclose(scriptFD);
            return -1;
        }
        char* final_build_script = malloc(sizeof(build_script) + MAX_PATH_SIZE);
        getFullPathForScript(build_script,final_build_script);
        if (final_build_script == NULL) {
            perror("Error resolving script path");
            return -1;
        }
        build_script = final_build_script;
        fclose(scriptFD);
    }

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
                    printf("Time to reload!");
                    executeBuildScript(build_script);
                    lastReload = eventTime;
                }
            }
        }
    }
    return 0;
}
