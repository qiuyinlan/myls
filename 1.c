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
    
    // 注册信号处理器
    struct sigaction sa_int, sa_tstp;
    
    // 设置SIGINT处理器
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    
    // 设置SIGTSTP处理器
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = 0;
    
    // 注册处理器
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
        
        // 只在这里打印提示符
        print_prompt();
        
        // 读取命令
        read_command(input_line);
        
        // 如果输入为空，继续下一次循环
        if (input_line[0] == '\0') {
            continue;
        }
        
        // 解析并执行命令
        args = parse_command(input_line, &background);
        status = execute_command(args, background);
        
        free_args(args);
    }
    
    return 0;
}