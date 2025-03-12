#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_CMD_LEN 1024
#define MAX_PIPES 10
#define MAX_DIR_LEN 4096

#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define MAGENTA "\033[0;35m"
#define CYAN "\033[1;32m"
#define WHITE "\033[0;37m"
#define BOLD "\033[1m"
#define UNDERLINE "\033[4m"
#define RESET "\033[0m"

extern void execute_command(char* cmd, int in_fd, int out_fd, int background);

void run_shell() {
    static char curr_dir[MAX_DIR_LEN] = "/";
    char cmd[MAX_CMD_LEN];
    uid_t uid = getuid();
    struct passwd* pw;

    if ((pw = getpwuid(uid)) == NULL) {
        perror("getpwuid");
        exit(EXIT_FAILURE);
    }

    while (true) {
        if (getcwd(curr_dir, MAX_DIR_LEN) == NULL) {
            perror("getcwd");
            exit(EXIT_FAILURE);
        }

        printf(CYAN "%s@LowShell:%s>" RESET, pw->pw_name, curr_dir);
        fflush(stdout);
        if (!fgets(cmd, MAX_CMD_LEN, stdin))
            break;
        cmd[strcspn(cmd, "\n")] = 0;

        char* commands[MAX_PIPES];
        int cmd_count = 0;
        char* token = strtok(cmd, "|");
        while (token) {
            commands[cmd_count++] = token;
            token = strtok(NULL, "|");
        }

        int in_fd = STDIN_FILENO;
        int fds[2];

        for (int i = 0; i < cmd_count; i++) {
            if (i < cmd_count - 1)
                pipe(fds);

            execute_command(commands[i], in_fd,
                            (i < cmd_count - 1) ? fds[1] : STDOUT_FILENO, 0);

            if (i > 0)
                close(in_fd);

            if (i < cmd_count - 1) {
                close(fds[1]);
                in_fd = fds[0];
            }
        }
    }
}