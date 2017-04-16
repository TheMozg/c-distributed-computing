#include <stdlib.h>
#include <argp.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>
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

    int fd_read[1024];
    int fd_writ[1024];
    for (size_t i = 0; i <= process_count; i++) {
        int fd[2];
        pipe(fd);
        fd_read[i] = fd[0];
        fd_writ[i] = fd[1];
    }
    int id = 0;
    for (size_t i = 1; i <= process_count; i++) {
        if (id == 0) {
            int pid = fork();
            if (pid == 0) {
                id = i;
                printf(log_started_fmt, (int)id, getpid(), getppid());
            }
        }
    }

    close(fd_writ[id]);
    for (size_t i = 0; i <= process_count; i++) {
        if (i != id) {
            close(fd_read[i]);
        }
    }
    char    string[] = "P_\n";
    string[1] = id;
    char    readbuffer[80];
    int nbytes;
    for (size_t i = 0; i <= process_count; i++) {
        if (i == id) {
            continue;
        }
        write(fd_writ[i], string, (strlen(string)+1));
        printf("P %d sent to: %d\n", id,(int)i);
    }
    for (size_t i = 0; i < process_count; i++) {
        nbytes = read(fd_read[id], readbuffer, strlen(string)+1);
        int id_r = readbuffer[1];
        printf("P %d received from: %d\n", id,id_r);
    }
    while( wait(NULL) > 0 );
    printf("P %d quit\n", id);
    exit (0);
}
