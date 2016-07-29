#include <iostream>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

/*
 * Copied from linux/include/linux/isolation.h
 */
/* Enable/disable or query task_isolation mode for TASK_ISOLATION kernels. */
#define PR_SET_TASK_ISOLATION		48
#define PR_GET_TASK_ISOLATION		49
# define PR_TASK_ISOLATION_ENABLE	(1 << 0)
# define PR_TASK_ISOLATION_USERSIG	(1 << 1)
# define PR_TASK_ISOLATION_SET_SIG(sig)	(((sig) & 0x7f) << 8)
# define PR_TASK_ISOLATION_GET_SIG(bits) (((bits) >> 8) & 0x7f)
# define PR_TASK_ISOLATION_NOSIG \
    (PR_TASK_ISOLATION_USERSIG | PR_TASK_ISOLATION_SET_SIG(0))

using namespace std;

int gettid()
{
    return (int) syscall(SYS_gettid);
}

void sigusr1(int signo)
{
    (void) signo;
    const char *msg = "sigusr\n";
    int ret = write(2, msg, strlen(msg));
    (void) ret;
    exit(0);
}

volatile int x;

int main(int argc, char *argv[])
{
    (void) argc; (void) argv;
    cpu_set_t cpuset;

    cout << "taskisol run" << endl;

    signal(SIGUSR1, sigusr1);

    CPU_ZERO(&cpuset);
    CPU_SET(1, &cpuset);
    sched_setaffinity(gettid(), sizeof(cpuset), &cpuset);

    mlockall(MCL_CURRENT);

    char *addr = (char *) mmap(nullptr, 4096, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!addr) {
        perror("addr");
        return -1;
    }

    if (argc > 1) {

        /*
         * Default SIGKILL mode
         */
        if (strcmp(argv[1], "default") == 0) {
            if (prctl(PR_SET_TASK_ISOLATION, PR_TASK_ISOLATION_ENABLE) < 0) {
                perror("prctl default");
                return -1;
            }
        }

        /*
         * The program completes when using USERSIG,
         * but actually no signal is delivered
         */
        if (strcmp(argv[1], "signal") == 0) {
            if (prctl(PR_SET_TASK_ISOLATION, PR_TASK_ISOLATION_USERSIG |
                      PR_TASK_ISOLATION_SET_SIG(SIGUSR1)) < 0) {
                perror("prctl sigusr");
                return -1;
            }
        }
    }

    if (argc > 2) {
        if (strcmp(argv[2], "syscall") == 0) {
            /*
             * Does not trigger a SIGKILL, instead writes the string on
             * stdout as normal
             */
            const char *msg = "hallo\n";
            int ret = write(2, msg, strlen(msg));
            (void) ret;
        }

        if (strcmp(argv[2], "pagefault") == 0) {
            /*
             * Triggers SIGKILL with default mode because the following
             * store produces a pagefault, even though the memory is valid
             */
            addr[0] = '\0';
        }
    }

    /*
     * Hog for a while to see if the task can survive (~10s on Intel i7)
     */
    for (long i = 0; i < 1E10; i++) {
        x = i;
    }

    /*
     * The task is allowed to call exit(), other syscall may produce SIGKILL.
     */
    exit(0);
    return 0;
}
