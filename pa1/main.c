#include <stdlib.h>
#include <argp.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/types.h>

#include "pa1.h"
#include "ipc.h"

static char doc[] = "ITMO Distributed Computing programming assignment #1";

static struct argp_option options[] = {
    {0, 'p', "PROCESS_COUNT", 0, "Number of child processes" },
    {0}
};

static error_t parse_opt (int key, char *arg, struct argp_state *state) {
    local_id *argument = state->input;
    switch (key)
    {
    case 'p':
        *argument = atoi(arg);
        if (*argument < 0 || *argument > MAX_PROCESS_ID){
            argp_failure(state, 1, 0,"PROCESS_COUNT must be between 0 and %d",MAX_PROCESS_ID);
        }
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };

int main (int argc, char **argv) {
    local_id process_count = 0;
    argp_parse (&argp, argc, argv, 0, 0, &process_count);
    process_count++;
    
    int fd_read[MAX_PROCESS_ID];
    int fd_writ[MAX_PROCESS_ID];
    for (local_id i = 0; i <= process_count; i++) {
        int fd[2];
        pipe(fd);
        fd_read[i] = fd[0];
        fd_writ[i] = fd[1];
    }
    local_id id = 0;
    for (local_id i = 1; i <= process_count; i++) {
        if (id == 0) {
            pid_t pid = fork();
            if (pid == 0) {
                id = i;
                printf(log_started_fmt, id, getpid(), getppid());
            }
        }
    }

    close(fd_writ[id]);
    for (local_id i = 0; i <= process_count; i++) {
        if (i != id) {
            close(fd_read[i]);
        }
    }
    for (local_id i = 0; i <= process_count; i++) {
        if (i != id) {
            write(fd_writ[i], &id, sizeof(local_id));
            printf("P %d sent to: %d\n", id, i);
        }
    }
    for (local_id i = 0; i < process_count; i++) {
        local_id id_r;
        read(fd_read[id], &id_r, sizeof(local_id));
        printf("P %d received from: %d\n", id, id_r);
    }
    if (id == 0){
        while( wait(NULL) > 0 );
    }
    printf("P %d quit\n", id);
    exit (EXIT_SUCCESS);
}
