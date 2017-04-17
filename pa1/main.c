#include <stdlib.h>
#include <argp.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#include "ipc.h"
#include "proc.h"
#include "logger.h"

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
    start_log();
    // Get number of child processes
    local_id process_count = 0;
    argp_parse (&argp, argc, argv, 0, 0, &process_count);

    // Increment to get total number of processes
    process_count++;

    proc_t this_process;
 
    // PARENT_ID does not have to be 0
    local_id id = start_procs( &this_process, process_count );

    // Send messages to all other processes
    if (id == PARENT_ID){
        Message msg;
        msg.s_header.s_magic = MESSAGE_MAGIC;
        msg.s_header.s_payload_len = sizeof(local_id);
        msg.s_header.s_type = STARTED;
        memcpy(&(msg.s_payload), &id, sizeof(local_id));
        send_multicast(&this_process, &msg);
    }

    // Wait for messages from all other processes
  /*`  for (local_id i = 1; i < process_count; i++) {
        Message msg;
        receive(&this_process, i, &msg);
        local_id id_r;
        memcpy(&id_r, msg.s_payload, msg.s_header.s_payload_len);
        log_output(fd_event, "P %d received from: %d\n", this_process.id, id_r);
    }
*/
    // Wait for children
    if (id == PARENT_ID){
        while( wait(NULL) > 0 );
        close_all_pipes(&this_process);
    }

    // Exit
    log_output(fd_event, "P %d quit\n", id);
    close_log();
    exit (EXIT_SUCCESS);
}
