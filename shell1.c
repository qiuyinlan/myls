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
#define MAX_ARGS 64
#define MAX_PIPES 10
#define MAX_DIR_LEN 4096
#define FILE_MODE 0644

// 设置信号处理函数，忽略特定信号
void signal_set() {
    // 忽略 SIGINT 信号 (Ctrl+C)
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        perror("signal SIGINT");
        exit(EXIT_FAILURE);
    }

    // 忽略 SIGCHLD 信号
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        perror("signal SIGCHLD");
        exit(EXIT_FAILURE);
    }

    // 忽略 SIGQUIT 信号
    if (signal(SIGQUIT, SIG_IGN) == SIG_ERR) {
        perror("signal SIGQUIT");
        exit(EXIT_FAILURE);
    }

    // 忽略 SIGTERM 信号
    if (signal(SIGTERM, SIG_IGN) == SIG_ERR) {
        perror("signal SIGTERM");
        exit(EXIT_FAILURE);
    }
}

// 处理输入输出重定向
static int handle_redirection(const char* filename, int flags, int std_fd) {
    int fd = open(filename, flags, FILE_MODE);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    if (dup2(fd, std_fd) < 0) {
        perror("dup2");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

// 执行单个命令
void execute_command(char* cmd, int in_fd, int out_fd, int background) {
    static char prev_dir[MAX_DIR_LEN] = ""; // 存储上一个目录
    char* args[MAX_ARGS];
    int argc = 0;
    char *infile = NULL, *outfile = NULL;
    int append = 0;

    // 解析命令行参数
    char* saveptr;
    char* token = strtok_r(cmd, " ", &saveptr);
    while (token) {
        if (strcmp(token, "&") == 0) {
            background = 1; // 后台执行
        } else if (strcmp(token, "<") == 0) {
            infile = strtok_r(NULL, " ", &saveptr); // 输入重定向
        } else if (strcmp(token, ">>") == 0) {
            outfile = strtok_r(NULL, " ", &saveptr); // 输出重定向（追加）
            append = 1;
        } else if (strcmp(token, ">") == 0) {
            outfile = strtok_r(NULL, " ", &saveptr); // 输出重定向（覆盖）
        } else {
            args[argc++] = token; // 收集命令参数
        }
        token = strtok_r(NULL, " ", &saveptr);
    }
    args[argc] = NULL;

    if (argc == 0)
        return;

    // 处理内置命令 cd
    if (strcmp(args[0], "cd") == 0) {
        char current_dir[MAX_DIR_LEN];
        if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
            perror("getcwd");
            return;
        }

        char* target = argc > 1 ? args[1] : getenv("HOME");
        if (target && strcmp(target, "-") == 0) {
            if (*prev_dir == '\0') {
                fprintf(stderr, "cd: OLDPWD not set\n");
                return;
            }
            target = prev_dir;
        }

        if (chdir(target) != 0) {
            perror("cd");
            return;
        }

        strncpy(prev_dir, current_dir, MAX_DIR_LEN - 1);
        prev_dir[MAX_DIR_LEN - 1] = '\0';
        return;
    }

    // 处理内置命令 exit
    if (strcmp(args[0], "exit") == 0) {
        exit(EXIT_SUCCESS);
    }

    // 创建子进程执行命令
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        // 子进程处理重定向
        if (infile && handle_redirection(infile, O_RDONLY, STDIN_FILENO) < 0) {
            exit(EXIT_FAILURE);
        }

        if (outfile) {
            int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
            if (handle_redirection(outfile, flags, STDOUT_FILENO) < 0) {
                exit(EXIT_FAILURE);
            }
        }

        // 处理管道输入输出
        if (in_fd != STDIN_FILENO) {
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }

        if (out_fd != STDOUT_FILENO) {
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }

        // 执行命令
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        // 父进程等待子进程
        if (!background) {
            waitpid(pid, NULL, 0);
        }
    }
}

// Shell 主循环
void run_shell() {
    static char curr_dir[MAX_DIR_LEN] = "/";
    char cmd[MAX_CMD_LEN];
    uid_t uid = getuid();
    struct passwd* pw;

    // 获取当前用户信息
    if ((pw = getpwuid(uid)) == NULL) {
        perror("getpwuid");
        exit(EXIT_FAILURE);
    }

    while (true) {
        // 获取当前工作目录
        if (getcwd(curr_dir, MAX_DIR_LEN) == NULL) {
            perror("getcwd");
            exit(EXIT_FAILURE);
        }

        // 显示提示符
        printf("\033[1;32m%s@LowShell:%s>\033[0m ", pw->pw_name, curr_dir);
        fflush(stdout);
        if (!fgets(cmd, MAX_CMD_LEN, stdin))
            break;
        cmd[strcspn(cmd, "\n")] = 0;

        // 解析管道命令
        char* commands[MAX_PIPES];
        int cmd_count = 0;
        char* token = strtok(cmd, "|");
        while (token) {
            commands[cmd_count++] = token;
            token = strtok(NULL, "|");
        }

        int in_fd = STDIN_FILENO;
        int fds[2];

        // 执行每个管道命令
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

// 主函数
int main() {
    signal_set(); // 设置信号处理
    run_shell();  // 运行Shell
    return 0;
}
