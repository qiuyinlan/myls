#include <stdio.h>      // 标准输入输出函数
#include <stdlib.h>     // 标准库函数，如exit()
#include <string.h>     // 字符串处理函数
#include <unistd.h>     // UNIX标准函数，如fork(), exec()
#include <sys/wait.h>   // waitpid()函数
#include <sys/types.h>  // 基本系统数据类型
#include <fcntl.h>      // 文件控制选项

#define MAX_INPUT_SIZE 1024  // 输入命令的最大长度
#define MAX_ARGS 64          // 命令参数的最大数量

/* 函数声明 */
void print_prompt();                  // 打印命令提示符
void parse_input(char *input, char **args);  // 解析用户输入
int execute_builtin(char **args);     // 执行内置命令
void execute_command(char **args);    // 执行外部命令
void handle_redirection(char **args); // 处理I/O重定向
int builtin_cd(char **args);          // 内置cd命令
int builtin_pwd();                    // 内置pwd命令
int builtin_help();                   // 内置help命令
int builtin_exit();                   // 内置exit命令

/**
 * 主函数 - Shell的入口点
 * 实现了Shell的主循环：显示提示符、读取命令、解析命令、执行命令
 */
int main() {
    char input[MAX_INPUT_SIZE];  // 存储用户输入的命令
    char *args[MAX_ARGS];        // 存储解析后的命令参数
    
    // Shell主循环
    while (1) {
        // 1. 打印命令提示符
        print_prompt();
        
        // 2. 读取用户输入
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            printf("\n");
            break; // 处理EOF (Ctrl+D)，退出Shell
        }
        
        // 去除输入末尾的换行符
        input[strcspn(input, "\n")] = '\0';
        
        // 如果输入为空，继续下一次循环
        if (strlen(input) == 0) {
            continue;
        }
        
        // 3. 解析命令行参数
        parse_input(input, args);
        
        // 如果解析后没有命令，继续下一次循环
        if (args[0] == NULL) {
            continue;
        }
        
        // 4. 先检查是否是内置命令
        int builtin_result = execute_builtin(args);
        if (builtin_result >= 0) {
            if (builtin_result == 0) {
                break; // exit命令，退出Shell
            }
            continue; // 其他内置命令，继续下一次循环
        }
        
        // 5. 执行外部命令
        execute_command(args);
    }
    
    return 0;
}

/**
 * 打印命令提示符
 * 显示当前工作目录和提示符符号
 */
void print_prompt() {
    char cwd[1024];  // 存储当前工作目录
    
    // 获取当前工作目录
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s$ ", cwd);  // 显示目录和提示符
    } else {
        printf("$ ");  // 获取目录失败，只显示提示符
    }
    fflush(stdout);  // 刷新输出缓冲区，确保提示符立即显示
}

/**
 * 解析输入命令
 * 将输入的命令行拆分为命令和参数
 * 
 * @param input 用户输入的命令行
 * @param args 存储解析后的命令和参数的数组
 */
void parse_input(char *input, char **args) {
    int i = 0;
    
    // 使用空格作为分隔符拆分命令行
    // strtok函数会修改原字符串，将分隔符替换为'\0'
    args[i] = strtok(input, " ");
    
    // 继续解析剩余的参数
    while (args[i] != NULL && i < MAX_ARGS - 1) {
        i++;
        args[i] = strtok(NULL, " ");  // 继续从上次的位置解析
    }
    
    args[i] = NULL;  // 参数列表以NULL结尾，这是exec函数的要求
}

/**
 * 执行内置命令
 * 检查并执行Shell内置的命令
 * 
 * @param args 命令和参数数组
 * @return 执行结果：0表示退出Shell，1表示继续，-1表示不是内置命令
 */
int execute_builtin(char **args) {
    // 检查是否是cd命令
    if (strcmp(args[0], "cd") == 0) {
        return builtin_cd(args);
    } 
    // 检查是否是pwd命令
    else if (strcmp(args[0], "pwd") == 0) {
        return builtin_pwd();
    } 
    // 检查是否是help命令
    else if (strcmp(args[0], "help") == 0) {
        return builtin_help();
    } 
    // 检查是否是exit命令
    else if (strcmp(args[0], "exit") == 0) {
        return builtin_exit();
    }
    
    return -1;  // 不是内置命令
}

/**
 * 执行外部命令
 * 创建子进程并执行外部程序
 * 
 * @param args 命令和参数数组
 */
void execute_command(char **args) {
    pid_t pid = fork();  // 创建子进程
    
    if (pid < 0) {
        // fork失败
        perror("fork failed");
    } else if (pid == 0) {
        // 子进程
        
        // 处理重定向
        handle_redirection(args);
        
        // 执行命令
        // execvp会在PATH环境变量指定的目录中查找命令
        if (execvp(args[0], args) == -1) {
            perror("command not found");
            exit(EXIT_FAILURE);  // 执行失败，退出子进程
        }
    } else {
        // 父进程
        int status;
        waitpid(pid, &status, 0);  // 等待子进程结束
    }
}

/**
 * 处理I/O重定向
 * 支持输入重定向(<)和输出重定向(>)
 * 
 * @param args 命令和参数数组，会被修改以移除重定向符号和文件名
 */
void handle_redirection(char **args) {
    int i;
    int in_redirect = -1;   // 输入重定向符号的位置
    int out_redirect = -1;  // 输出重定向符号的位置
    
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
        // dup2将文件描述符fd复制到STDIN_FILENO(0)
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("input redirection");
            exit(EXIT_FAILURE);
        }
        
        close(fd);  // 关闭原文件描述符
        
        // 从参数列表中移除重定向符号和文件名
        args[in_redirect] = NULL;
    }
    
    // 处理输出重定向
    if (out_redirect != -1) {
        // 打开输出文件
        // O_WRONLY: 只写模式
        // O_CREAT: 如果文件不存在则创建
        // O_TRUNC: 如果文件存在则截断为0长度
        // 0644: 文件权限(rw-r--r--)
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
        
        close(fd);  // 关闭原文件描述符
        
        // 从参数列表中移除重定向符号和文件名
        args[out_redirect] = NULL;
    }
}

/**
 * 内置命令: cd
 * 改变当前工作目录
 * 
 * @param args 命令和参数数组
 * @return 1表示继续Shell循环
 */
int builtin_cd(char **args) {
    if (args[1] == NULL) {
        // 没有参数，切换到HOME目录
        if (chdir(getenv("HOME")) != 0) {
            perror("cd");  // 切换失败
        }
    } else {
        // 切换到指定目录
        if (chdir(args[1]) != 0) {
            perror("cd");  // 切换失败
        }
    }
    return 1;  // 继续Shell循环
}

/**
 * 内置命令: pwd
 * 打印当前工作目录
 * 
 * @return 1表示继续Shell循环
 */
int builtin_pwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);  // 打印当前目录
    } else {
        perror("getcwd");  // 获取目录失败
    }
    return 1;  // 继续Shell循环
}

/**
 * 内置命令: help
 * 显示Shell的帮助信息
 * 
 * @return 1表示继续Shell循环
 */
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
    return 1;  // 继续Shell循环
}

/**
 * 内置命令: exit
 * 退出Shell
 * 
 * @return 0表示退出Shell循环
 */
int builtin_exit() {
    return 0;  // 退出Shell循环
}
