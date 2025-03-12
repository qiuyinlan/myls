#include <stdio.h>
#include <stdlib.h>

extern void run_shell();
extern void signal_set();

int main() {
    signal_set();
    run_shell();
    return 0;
}