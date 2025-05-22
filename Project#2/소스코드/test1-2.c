#include "types.h"
#include "stat.h"
#include "user.h"

#define ITERATIONS 10000000

int main(void) {
    int pid;
    printf(1, "start scheduler_test\n");

    if ((pid = fork()) == 0) {
        // Set process information: start in queue 1 with 500 total ticks allowed
        set_proc_info(1, 0, 0, 0, 500);

        while(1);
        exit();
    }

    wait();  // Wait for the child process to finish
    printf(1, "end of scheduler_test\n");
    exit();
}
