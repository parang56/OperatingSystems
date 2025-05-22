#include "types.h"
#include "stat.h"
#include "user.h"

#define ITERATIONS 10000000

int main(void) {
    printf(1, "start scheduler_test\n");

    for (int i = 0; i < 3; i++) {
        if (fork() == 0) {
            set_proc_info(2, 0, 0, 0, 300);

            while(1);
            exit();
        }
    }

    // Parent process waits for all children to finish
    for (int i = 0; i < 3; i++) {
        wait();
    }

    printf(1, "end of scheduler_test\n");
    exit();
}

