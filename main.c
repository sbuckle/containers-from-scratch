#define _GNU_SOURCE
#include <err.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>

#define STACK_SIZE (1024 * 1024) /* Stack size for cloned child */

static int childFunc(void *arg) {
        struct utsname uts;

        /* Change hostname in UTS namespace in child */
        if (sethostname(arg, strlen(arg)) == -1)
                err(EXIT_FAILURE, "sethostname");

        /* Retrieve and display the hostname */
        if (uname(&uts) == -1)
                err(EXIT_FAILURE, "uname");
        printf("uts.nodename in child: %s\n", uts.nodename);

        if (execl("/proc/self/exe", "./main", "child", NULL)) {
                err(EXIT_FAILURE, "execl");
        }

        return 0; /* Child terminates now */
}

int main(int argc, char *argv[])
{
        char *stack;
        char *stackTop;
        pid_t pid;
        struct utsname uts;

        if (argc > 1 && strcmp(argv[1], "child") == 0) {
                printf("Child running\n");
                sleep(20);
                exit(EXIT_SUCCESS);
        }

        stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);

        if (stack == MAP_FAILED)
                err(EXIT_FAILURE, "mmap");

        stackTop = stack + STACK_SIZE; /* Assume stack grows downards */

        /* Create child that has its own UTS namespace;
           child commences execution in childFunc() */
        pid = clone(childFunc, stackTop, CLONE_NEWUTS | SIGCHLD, "container");
        if (pid == -1)
                err(EXIT_FAILURE, "clone");
        printf("clone() returned %jd\n", (intmax_t) pid);

        /* Parent falls through to here */

        sleep(1); /* Give child time to change its hostname */

        if (uname(&uts) == -1)
                err(EXIT_FAILURE, "uname");

        if (waitpid(pid, NULL, 0) == -1)
                err(EXIT_FAILURE, "waitpid");
        printf("Child has terminated\n");

        exit(EXIT_SUCCESS);
}