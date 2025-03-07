#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64

/* 函数声明 */
void print_prompt();
void parse_input(char *input, char **args);
int execute_builtin(char **args);
void execute_command(char **args);
void handle_redirection(char **args);
int builtin_cd(char **args);
int builtin_pwd();
int builtin_help();
int builtin_exit();

/* 主函数 */
int main() {
    char input[MAX_INPUT_SIZE];
    char *args[MAX_ARGS];
    
    while (1) {
        // 打印提示符
        print_prompt();
        
        // 读取用户输入
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            printf("\n");
            break; // 处理EOF (Ctrl+D)
        }
        
        // 去除末尾的换行符
        input[strcspn(input, "\n")] = '\0';
        
        if (strlen(input) == 0) {
            continue; // 空命令
        }
        
        // 解析命令
        parse_input(input, args);
        
        if (args[0] == NULL) {
            continue; // 解析后为空
        }
        
        // 先检查是否是内置命令
        int builtin_result = execute_builtin(args);
        if (builtin_result >= 0) {
            if (builtin_result == 0) {
                break; // exit命令
            }
            continue; // 其他内置命令
        }
        
        // 执行外部命令
        execute_command(args);
    }
    
    return 0;
}

/* 打印命令提示符 */
void print_prompt() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s$ ", cwd);
    } else {
        printf("$ ");
    }
    fflush(stdout);
}

/* 解析输入命令 */
void parse_input(char *input, char **args) {
    int i = 0;
    
    // 使用空格作为分隔符拆分命令行
    args[i] = strtok(input, " ");
    
    while (args[i] != NULL && i < MAX_ARGS - 1) {
        i++;
        args[i] = strtok(NULL, " ");
    }
    
    args[i] = NULL; // 参数列表以NULL结尾
}

/* 执行内置命令 */
int execute_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        return builtin_cd(args);
    } else if (strcmp(args[0], "pwd") == 0) {
        return builtin_pwd();
    } else if (strcmp(args[0], "help") == 0) {
        return builtin_help();
    } else if (strcmp(args[0], "exit") == 0) {
        return builtin_exit();
    }
    
    return -1; // 不是内置命令
}

/* 执行外部命令 */
void execute_command(char **args) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
    } else if (pid == 0) {
        // 子进程
        
        // 处理重定向
        handle_redirection(args);
        
        if (execvp(args[0], args) == -1) {
            perror("command not found");
            exit(EXIT_FAILURE);
        }
    } else {
        // 父进程
        int status;
        waitpid(pid, &status, 0); // 等待子进程结束
    }
}

/* 处理I/O重定向 */
void handle_redirection(char **args) {
    int i;
    int in_redirect = -1;  // 输入重定向位置
    int out_redirect = -1; // 输出重定向位置
    
    // 查找重定向符号
    for (i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            in_redirect = i;
        } else if (strcmp(args[i], ">") == 0) {
            out_redirect = i;
        }
    }
    
    // 处理输入重定向
    if (in_redirect != -1) {
        // 打开输入文件
        int fd = open(args[in_redirect + 1], O_RDONLY);
        if (fd < 0) {
            perror("open input file");
            exit(EXIT_FAILURE);
        }
        
        // 重定向标准输入
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("input redirection");
            exit(EXIT_FAILURE);
        }
        
        close(fd);
        
        // 从参数列表中移除重定向符号和文件名
        args[in_redirect] = NULL;
    }
    
    // 处理输出重定向
    if (out_redirect != -1) {
        // 打开输出文件
        int fd = open(args[out_redirect + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open output file");
            exit(EXIT_FAILURE);
        }
        
        // 重定向标准输出
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("output redirection");
            exit(EXIT_FAILURE);
        }
        
        close(fd);
        
        // 从参数列表中移除重定向符号和文件名
        args[out_redirect] = NULL;
    }
}

/* 内置命令: cd */
int builtin_cd(char **args) {
    if (args[1] == NULL) {
        // 没有参数，切换到HOME目录
        if (chdir(getenv("HOME")) != 0) {
            perror("cd");
        }
    } else {
        if (chdir(args[1]) != 0) {
            perror("cd");
        }
    }
    return 1; // 继续Shell循环
}

/* 内置命令: pwd */
int builtin_pwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("getcwd");
    }
    return 1;
}

/* 内置命令: help */
int builtin_help() {
    printf("MyShell - 简单的Shell实现\n");
    printf("内置命令:\n");
    printf("  cd [dir] - 切换目录\n");
    printf("  pwd      - 显示当前目录\n");
    printf("  help     - 显示帮助信息\n");
    printf("  exit     - 退出Shell\n");
    printf("\n");
    printf("其他功能:\n");
    printf("  cmd > file  - 输出重定向\n");
    printf("  cmd < file  - 输入重定向\n");
    return 1;
}

/* 内置命令: exit */
int builtin_exit() {
    return 0; // 退出Shell
}
