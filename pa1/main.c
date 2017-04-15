#include <stdlib.h>
#include <argp.h>

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

    printf ("process_count = %d\n", process_count);

    exit (0);
}
