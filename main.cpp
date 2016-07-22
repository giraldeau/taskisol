#include <iostream>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

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

void sigkill(int signo)
{
    (void) signo;
    const char *msg = "sigsegv\n";
    write(2, msg, strlen(msg));
    exit(0);
}

volatile int x;

int main(int argc, char *argv[])
{
    (void) argc; (void) argv;
    cpu_set_t cpuset;

    cout << "taskisol run" << endl;

    //signal(SIGKILL, sigkill);

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

    if (prctl(PR_SET_TASK_ISOLATION, PR_TASK_ISOLATION_ENABLE) < 0) {
        perror("prctl");
        return -1;
    }

    if (argc > 1) {
        if (strcmp(argv[1], "syscall") == 0) {
            // does not produce a segfault, writes the string
            const char *msg = "hallo\n";
            write(2, msg, strlen(msg));
        }

        if (strcmp(argv[1], "pagefault") == 0) {
            // does trigger SIGKILL
            addr[0] = '\0';
        }
    }

    for (int i = 0; i < 1E10; i++) {
        x = i;
    }
    return 0;
}
