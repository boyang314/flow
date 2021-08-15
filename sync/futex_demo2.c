#include <stdio.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <sys/time.h>

static int futex(uint32_t *uaddr, int futex_op, uint32_t val, const struct timespec *timeout, uint32_t *uaddr2, uint32_t val3)
{
    return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

void wake_futex_blocking(int* futex_addr) {
    while (1) {
        int futex_rc = futex(futex_addr, FUTEX_WAKE, 1, NULL, NULL, 0);
        if (futex_rc == -1) {
            perror("futex wake");
            exit(1);
        } else if (futex_rc > 0) {
            return;
        }
    }
}

void wait_on_futex_value(int* futex_addr, int val) {
    while (1) {
        int futex_rc = futex(futex_addr, FUTEX_WAIT, val, NULL, NULL, 0);
        if (futex_rc == -1) {
            if (errno != EAGAIN) {
                perror("futex");
                exit(1);
            }
        } else if (futex_rc == 0) {
            if (*futex_addr == val) {
                // This is a real wakeup.
                return;
            }
        } else {
            abort();
        }
    }
}

int main(int argc, char** argv) {
    int shm_id = shmget(IPC_PRIVATE, 4096, IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }
    int* shared_data = shmat(shm_id, NULL, 0);
    *shared_data = 0;

    int forkstatus = fork();
    if (forkstatus < 0) {
        perror("fork");
        exit(1);
    }

    if (forkstatus == 0) {
        // Child process

        printf("child waiting for A\n");
        wait_on_futex_value(shared_data, 0xA);

        printf("child writing B\n");
        // Write 0xB to the shared data and wake up parent.
        *shared_data = 0xB;
        wake_futex_blocking(shared_data);
    } else {
        // Parent process.

        printf("parent writing A\n");
        // Write 0xA to the shared data and wake up child.
        *shared_data = 0xA;
        wake_futex_blocking(shared_data);

        printf("parent waiting for B\n");
        wait_on_futex_value(shared_data, 0xB);

        // Wait for the child to terminate.
        wait(NULL);
        shmdt(shared_data);
    }

    return 0;
}
