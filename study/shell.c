#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64
#define MAX_PATH_LEN 256

// 全局变量
char current_dir[MAX_PATH_LEN];
char previous_dir[MAX_PATH_LEN];
int shell_error = 0;

// 函数声明
void print_prompt();
void show_welcome();
void read_command(char *line);
char **parse_command(char *line, int *background);
int execute_command(char **args, int background);
int handle_builtin(char **args);
int redirect_io(char **args);
int setup_pipes(char **args);
int cmd_cd(char **args);
int cmd_exit(char **args);
int cmd_help(char **args);
int cmd_clear(char **args);
char **split_line(char *line, char *delim);
void free_args(char **args);
void ignore_signals();
void sigint_handler(int sig);
void sigtstp_handler(int sig);

// 显示欢迎信息
void show_welcome() {
    printf("\n");
    printf("  __  __         ____  _          _ _ \n");
    printf(" |  \\/  |_   _  / ___|| |__   ___| | |\n");
    printf(" | |\\/| | | | | \\___ \\| '_ \\ / _ \\ | |\n");
    printf(" | |  | | |_| |  ___) | | | |  __/ | |\n");
    printf(" |_|  |_|\\__, | |____/|_| |_|\\___|_|_|\n");
    printf("         |___/                        \n");
    printf("\n");

}

// 显示命令提示符
void print_prompt() {
    // 获取当前目录的基本名称（而不是完整路径）
    char *dir_name = strrchr(current_dir, '/');
    if (dir_name == NULL) {
        dir_name = current_dir;
    } else {
        dir_name++; // 跳过'/'字符
    }
    
    // 只显示用户名和当前目录名
    struct passwd *pw = getpwuid(getuid());
    const char *username = pw ? pw->pw_name : "user";
    
    printf("\033[1;32m%s\033[0m:\033[1;34m%s\033[0m$ ", 
           username, dir_name);
    fflush(stdout);
}

// 读取命令
void read_command(char *line) {
    // 确保不会在这个函数中打印提示符
    if (fgets(line, MAX_INPUT_SIZE, stdin) == NULL) {
        if (feof(stdin)) {
            printf("exit\n");
            exit(0);
        } else {
            perror("读取命令失败");
            exit(1);
        }
    }
    
    // 移除末尾的换行符
    size_t len = strlen(line);
    if (len > 0 && line[len-1] == '\n') {
        line[len-1] = '\0';
    }
    
    // 不要在这里打印任何提示符
}

// 分割字符串
char **split_line(char *line, char *delim) {
    int bufsize = MAX_ARGS;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;
    
    if (!tokens) {
        fprintf(stderr, "myshell: 内存分配错误\n");
        exit(EXIT_FAILURE);
    }
    
    token = strtok(line, delim);
    while (token != NULL) {
        tokens[position] = token;
        position++;
        
        if (position >= bufsize) {
            bufsize += MAX_ARGS;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "myshell: 内存分配错误\n");
                exit(EXIT_FAILURE);
            }
        }
        
        token = strtok(NULL, delim);
    }
    tokens[position] = NULL;
    return tokens;
}

// 解析命令
char **parse_command(char *line, int *background) {
    char **args;
    
    // 检查后台运行符号 &
    if (strchr(line, '&') != NULL) {
        *background = 1;
        // 移除 & 符号
        char *amp = strchr(line, '&');
        *amp = '\0';
    } else {
        *background = 0;
    }
    
    // 分割命令行
    args = split_line(line, " \t\r\n\a");
    return args;
}

// 处理内置命令
int handle_builtin(char **args) {
    if (args[0] == NULL) {
        return 0;
    }
    
    if (strcmp(args[0], "cd") == 0) {
        return cmd_cd(args);
    } else if (strcmp(args[0], "exit") == 0) {
        return cmd_exit(args);
    } else if (strcmp(args[0], "help") == 0) {
        return cmd_help(args);
    } else if (strcmp(args[0], "clear") == 0) {
        return cmd_clear(args);
    } else if (strcmp(args[0], "echo") == 0 && args[1] && args[1][0] == '$') {
        char *var_name = args[1] + 1;  // 跳过$符号
        char *env_val = getenv(var_name);
        if (env_val) {
            printf("%s\n", env_val);
        } else {
            printf("\n");  // 变量不存在时输出空行
        }
        return 0;
    } else if (strcmp(args[0], "env") == 0) {
        // 显示所有环境变量
        extern char **environ;
        for (char **env = environ; *env != NULL; env++) {
            printf("%s\n", *env);
        }
        return 0;
    } else if (strcmp(args[0], "export") == 0) {
        // 设置环境变量
        if (args[1]) {
            char *equals = strchr(args[1], '=');
            if (equals) {
                *equals = '\0';  // 分割变量名和值
                setenv(args[1], equals + 1, 1);
            }
        }
        return 0;
    }
    
    return 1; // 不是内置命令
}

// cd命令
int cmd_cd(char **args) {
    if (args[1] == NULL) {
        // 没有参数，切换到HOME目录
        char *home = getenv("HOME");
        if (home == NULL) {
            printf("myshell: 无法获取HOME目录\n");
            shell_error = 1;
            return 0;
        }
        
        strcpy(previous_dir, current_dir);
        if (chdir(home) != 0) {
            perror("cd");
            shell_error = 1;
        } else {
            getcwd(current_dir, MAX_PATH_LEN);
        }
    } else if (strcmp(args[1], "-") == 0) {
        // 切换到上一个目录
        if (chdir(previous_dir) != 0) {
            perror("cd");
            shell_error = 1;
        } else {
            char temp[MAX_PATH_LEN];
            strcpy(temp, current_dir);
            strcpy(current_dir, previous_dir);
            strcpy(previous_dir, temp);
            printf("%s\n", current_dir);
        }
    } else {
        // 切换到指定目录
        strcpy(previous_dir, current_dir);
        if (chdir(args[1]) != 0) {
            perror("cd");
            shell_error = 1;
        } else {
            getcwd(current_dir, MAX_PATH_LEN);
        }
    }
    
    return 0;
}

// exit命令
int cmd_exit(char **args) {
    return 0; // 返回0会导致shell退出
}

// help命令
int cmd_help(char **args) {
    printf("MyShell - 一个简单的shell实现\n");
    printf("内置命令:\n");
    printf("  cd [目录]   - 切换工作目录\n");
    printf("  exit        - 退出shell\n");
    printf("  help        - 显示帮助信息\n");
    printf("  clear       - 清屏\n");
    printf("\n");
    printf("其他功能:\n");
    printf("  命令 &      - 在后台运行命令\n");
    printf("  命令1 | 命令2 - 管道\n");
    printf("  命令 > 文件  - 输出重定向\n");
    printf("  命令 < 文件  - 输入重定向\n");
    return 0;
}

// clear命令
int cmd_clear(char **args) {
    system("clear");
    return 0;
}

// 处理重定向
int redirect_io(char **args) {
    int i;
    int fd;
    
    for (i = 0; args[i] != NULL; i++) {
        // 输出重定向 >
        if (strcmp(args[i], ">") == 0) {
            if (args[i+1] == NULL) {
                fprintf(stderr, "myshell: 重定向后缺少文件名\n");
                return 1;
            }
            
            fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("open");
                return 1;
            }
            
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                return 1;
            }
            
            close(fd);
            args[i] = NULL; // 移除重定向符号和文件名
            args[i+1] = NULL;
        }
        
        // 输入重定向 <
        if (strcmp(args[i], "<") == 0) {
            if (args[i+1] == NULL) {
                fprintf(stderr, "myshell: 重定向后缺少文件名\n");
                return 1;
            }
            
            fd = open(args[i+1], O_RDONLY);
            if (fd == -1) {
                perror("open");
                return 1;
            }
            
            if (dup2(fd, STDIN_FILENO) == -1) {
                perror("dup2");
                return 1;
            }
            
            close(fd);
            args[i] = NULL;
            args[i+1] = NULL;
        }
        
        // 追加重定向 >>
        if (strcmp(args[i], ">>") == 0) {
            if (args[i+1] == NULL) {
                fprintf(stderr, "myshell: 重定向后缺少文件名\n");
                return 1;
            }
            
            fd = open(args[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1) {
                perror("open");
                return 1;
            }
            
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                return 1;
            }
            
            close(fd);
            args[i] = NULL;
            args[i+1] = NULL;
        }
    }
    
    return 0;
}

// 处理管道
int setup_pipes(char **args) {
    int i;
    int pipe_pos = -1;
    
    // 查找管道符号
    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "|") == 0) {
            pipe_pos = i;
            break;
        }
    }
    
    if (pipe_pos == -1) {
        return 0; // 没有管道
    }
    
    // 分割命令
    args[pipe_pos] = NULL;
    char **cmd2 = &args[pipe_pos + 1];
    
    int pipefd[2];
    pid_t pid;
    
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }
    
    pid = fork();
    
    if (pid == 0) {
        // 子进程执行第二个命令
        close(pipefd[1]); // 关闭写端
        
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        
        close(pipefd[0]);
        
        if (execvp(cmd2[0], cmd2) == -1) {
            printf("myshell: command not found: %s\n", cmd2[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        // 父进程执行第一个命令
        close(pipefd[0]); // 关闭读端
        
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            return 1;
        }
        
        close(pipefd[1]);
        
        if (execvp(args[0], args) == -1) {
            printf("myshell: command not found: %s\n", args[0]);
            exit(EXIT_FAILURE);
        }
    }
    
    return 0;
}

// 执行命令
int execute_command(char **args, int background) {
    pid_t pid;
    int status;
    
    if (args[0] == NULL) {
        return 1;
    }
    
    // 检查是否是内置命令
    if (handle_builtin(args) == 0) {
        return 1;
    }
    
    // 创建子进程执行命令
    pid = fork();
    
    if (pid == 0) {
        // 子进程
        
        // 处理重定向
        if (redirect_io(args) != 0) {
            exit(EXIT_FAILURE);
        }
        
        // 处理管道
        if (setup_pipes(args) != 0) {
            exit(EXIT_FAILURE);
        }
        
        // 执行命令
        if (execvp(args[0], args) == -1) {
            printf("myshell: command not found: %s\n", args[0]);
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        // 创建进程失败
        perror("fork失败");
        shell_error = 1;
    } else {
        // 父进程
        if (!background) {
            // 前台运行，等待子进程结束
            waitpid(pid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                shell_error = 1;
            }
        } else {
            // 后台运行，打印进程ID
            printf("[1] %d\n", pid);
        }
    }
    
    return 1;
}

// 忽略信号
void ignore_signals() {
    signal(SIGINT, SIG_IGN);  // 忽略Ctrl+C
}

// 释放参数数组
void free_args(char **args) {
    if (args != NULL) {
        free(args);
    }
}

// SIGINT (Ctrl+C) 处理函数
void sigint_handler(int sig) {
    printf("\n");  // 只打印一个换行符
    print_prompt();  // 重新显示提示符
}

// SIGTSTP (Ctrl+Z) 处理函数
void sigtstp_handler(int sig) {
    printf("\n退出Shell程序...\n");
    exit(0);
}

// 主函数
int main() {
    char input_line[MAX_INPUT_SIZE];
    char **args;
    int status = 1;
    int background = 0;
    
    // 获取初始工作目录
    getcwd(current_dir, MAX_PATH_LEN);
    strcpy(previous_dir, current_dir);
    
    // 显示欢迎信息
    show_welcome();
    
    // 注册信号处理器（移除日志输出）
    struct sigaction sa_int, sa_tstp;
    
    // 设置SIGINT处理器
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    
    // 设置SIGTSTP处理器
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = 0;
    
    // 注册处理器（移除成功日志）
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("无法注册SIGINT信号处理器");
    }
    
    if (sigaction(SIGTSTP, &sa_tstp, NULL) == -1) {
        perror("无法注册SIGTSTP信号处理器");
    }
    
    // 主循环
    while (status) {
        // 清除之前的错误状态
        shell_error = 0;
        
        // 只在这里打印提示符，不要在其他地方打印
        print_prompt();
        
        // 读取命令
        read_command(input_line);
        
        // 如果输入为空，继续下一次循环，但不要再次打印提示符
        if (input_line[0] == '\0') {
            continue;
        }
        
        // 解析并执行命令
        args = parse_command(input_line, &background);
        status = execute_command(args, background);
        
        free_args(args);
        
        // 不要在这里打印提示符
    }
    
    return 0;
}