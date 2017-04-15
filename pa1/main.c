#include <stdlib.h>
#include <argp.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include "pa1.h"

static char doc[] = "ITMO Distributed Computing programming assignment #1";

static struct argp_option options[] = {
    {0, 'p', "PROCESS_COUNT", 0, "Number of child processes" },
    {0}
};

static error_t parse_opt (int key, char *arg, struct argp_state *state) {
    int *argument = state->input;
    switch (key)
    {
    case 'p':
        *argument = atoi(arg);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };

int main (int argc, char **argv) {
    int process_count = 0;
    argp_parse (&argp, argc, argv, 0, 0, &process_count);

    int id = 0;
    for (size_t i = 1; i <= process_count; i++) {
        if (id == 0) {
            int pid = fork();
            if (pid == 0) {
                id = i;
                printf(log_started_fmt, (int)id, getpid(), getppid());
                sleep(5);
            }
        }
    }

    for (size_t i = 0; i < process_count; i++) {
        wait(NULL);
    }

    exit (0);
}
