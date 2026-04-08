/*******************************************************************************
 * Name        : pfind.c
 * Author      : Michael Logozzo
 * Pledge      : I pledge my honor that I have abided by the Stevens Honor System
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <pwd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#define BLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"
volatile sig_atomic_t interrupted = 0;

void sigint_handler(int sig)
{
    interrupted = 1;
}

void minishell()
{
    char cwd[PATH_MAX];
    char str[1024];
    // SIGINT stuff
    struct sigaction sa;
    while(1) // sigint is broken, Try to talk with tas
    {
        memset(&sa, 0, sizeof(sa)); // Struct set to 0 so that the current function is wiped
        sa.sa_handler = &sigint_handler;
        if(sigaction(SIGINT, &sa, NULL) == -1)
        {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }
        if(interrupted)
        {
            interrupted = 0;
            printf("\n");
            continue;
        }
        if(!getcwd(cwd, sizeof(cwd)))
        {
            perror("getcwd");
            strcpy(cwd, "?"); // Insert "huh" meme
        }
        printf("%s[%s]%s> ", BLUE, cwd, DEFAULT);
        fgets(str, sizeof(str), stdin);
        str[strcspn(str, "\n")] = '\0'; // Check for null terminator
        char *args[64];
        int argc = 0;
        char *token = strtok(str, " ");
        while (token && argc < 63)
        {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL;
        if(argc == 0)
        {
            continue;
        }
        if(strcmp(args[0], "cd") == 0) // Change Directory, I'm a dumbass, I had a new thing to do half of this.
        {
            if(argc == 1)
            {
                char *home = getenv("HOME");
                if(home) // Checks for null home
                {
                    if(chdir(home) == -1)
                    {
                        perror("chdir");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    fprintf(stderr, "Error: Cannot change directory to %s. %s\n", home, strerror(errno));
                }
            }
            else if(argc == 2)
            {
                char *dir = args[1];
                if(dir[0] == '~')
                {
                    char path[PATH_MAX];
                    snprintf(path, sizeof(path), "%s%s", getenv("HOME"), dir + 1);
                    dir = path;
                }
                if(chdir(dir) == -1)
                {
                    fprintf(stderr, "Error: Cannot change directory to %s. %s\n", dir, strerror(errno));
                }
            }
            else
            {
                fprintf(stderr, "Error: Too many arguments to cd.\n");
            }
        }
        else if(strcmp(args[0], "exit") == 0) // Exit
        {
            exit(EXIT_SUCCESS);
        }
        else if(strcmp(args[0], "pwd") == 0) // Print working directory
        {
            if(!getcwd(cwd, sizeof(cwd)))
            {
                perror("getcwd");
            }
            else
            {
                printf("%s\n", cwd);
            }
        }
        else if(strcmp(args[0], "lf") == 0) // ls without command line args
        {
            DIR *dir = opendir(".");
            if(dir)
            {
                struct dirent *entry;
                while((entry = readdir(dir)) != NULL)
                {
                    if(entry->d_name[0] != '.')
                    {
                        printf("%s\n", entry->d_name);
                    }
                }
                closedir(dir);
            }
            else
            {
                perror("opendir"); // Don't believe I should exit since no syscall, but I do feel like it should state error
            }
        }
        else if(strcmp(args[0], "lp") == 0) // list processes, not in Linux
        {
            DIR *dir = opendir("/proc");
            if(dir)
            {
                struct dirent *entry;
                while((entry = readdir(dir)) != NULL)
                {
                    if(entry->d_type == DT_DIR && isdigit(entry->d_name[0]))
                    {
                        char path[PATH_MAX];
                        snprintf(path, sizeof(path), "/proc/%s/cmdline", entry->d_name);
                        FILE *cmdfile = fopen(path, "r");
                        if(cmdfile)
                        {
                            char cmd[1024] = "";
                            fgets(cmd, sizeof(cmd), cmdfile);
                            struct passwd *pw = getpwuid(atoi(entry->d_name));
                            printf("%s %s %s\n", entry->d_name, pw ? pw->pw_name : "", cmd);
                            fclose(cmdfile);
                        }
                        else
                        {
                            printf("%s %s \n", entry->d_name, "");
                        }
                    }
                }
                closedir(dir);
            }
            else
            {
                perror("opendir"); // Again I don't believe it syscalls, but the error should be there
            }
        }
        else // This is the exec and fork case for any commands not listed. The "send the child to the mines" case if I may.
        {
            pid_t pid = fork();
            if(pid == -1)
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            else if(pid == 0) // Child
            {
                if(execvp(args[0], args) == -1) // I believe I asked in class if this was a syscall. He said yes man7 said no. Ask in OH
                {
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
                fprintf(stderr,"execv: No such file or directory.\n");
                exit(EXIT_FAILURE);
            }
            else // Parent, I need to make sure sigint does nothing which it should be doing but idk
            {
                memset(&sa, 0, sizeof(sa)); // Struct set to 0 so that the current function is wiped
                sa.sa_handler = SIG_IGN;
                if(sigaction(SIGINT, &sa, NULL) == -1)
                {
                    perror("sigaction");
                    exit(EXIT_FAILURE);
                }
                if(wait(NULL) == -1)
                {
                    perror("wait");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}

int main(int argc, char *argv)
{
    if(argc != 1)
    {
        printf("Usage: ./minishell\n");
        exit(EXIT_FAILURE);
    }
    minishell();
    return 0;
}