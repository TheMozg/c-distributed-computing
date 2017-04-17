#include <stdlib.h>
#include <argp.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

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

Message create_message ( MessageType type, char* contents ) {
    Message msg;

    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = strlen(contents);
    msg.s_header.s_type = type;
    memcpy(&(msg.s_payload), contents, strlen(contents));

    return msg;
}

static struct argp argp = { options, parse_opt, 0, doc };

int wait_for_all_messages ( proc_t* proc, int counter_done, MessageType status ) {

    int counter_started = 0;
    int procs_to_wait = proc->process_count - 2; // Don't wait PARENT process and itself
    int current_counter = 0;

    do {
        for (local_id i = 0; i < proc->process_count; i++) {
            if(i != proc->id && (i != PARENT_ID || proc->id == PARENT_ID)){
                Message msg;
                receive(proc, i, &msg);
                if ( status == STARTED ) {
                    if (msg.s_header.s_type == STARTED) counter_started++;
                }
                if (msg.s_header.s_type == DONE) counter_done++;
            }
        } 

    current_counter = ( status == STARTED ) ? counter_started : counter_done;
    } while ( current_counter < procs_to_wait ); // To ensure that we got all STARTED messages

    return counter_done;
}

int wait_for_all_started ( proc_t* proc ) {
    return wait_for_all_messages ( proc, 0, STARTED );
}

void wait_for_all_done ( proc_t* proc, int counter_done ) {
    wait_for_all_messages ( proc, counter_done, DONE );
}

void send_started ( proc_t* proc, char* buf ) {
    Message msg = create_message ( STARTED, buf );
    send_multicast(proc, &msg);
}

void send_done ( proc_t* proc, char* buf ) {
    Message msg = create_message ( DONE, buf );
    send_multicast(proc, &msg);
}

void children_routine ( proc_t* proc, char* buf ) {
    
    send_started ( proc, buf );

    int counter_done = wait_for_all_started ( proc );

    send_done ( proc, buf );
    log_received_all_started ( proc->id );
    
    wait_for_all_done ( proc, counter_done );

    log_received_all_done ( proc->id );
    log_done ( proc->id );
}

void parent_routine ( proc_t* proc, char* buf ) {
    int counter_done = wait_for_all_started ( proc );
    wait_for_all_done ( proc, counter_done );
    close_log();
}

int main (int argc, char **argv) {
    start_log();
    // Get number of child processes
    local_id process_count = 0;
    argp_parse (&argp, argc, argv, 0, 0, &process_count);

    // Increment to get total number of processes
    process_count++;

    proc_t this_process;
 
    // PARENT_ID does not have to be 0
    char * buf = start_procs( &this_process, process_count );

    if (this_process.id != PARENT_ID) {
        children_routine ( &this_process, buf );
    } else parent_routine ( &this_process, buf );

    free(buf);
    // Exit
    //log_output(fd_event, "P %d quit\n", this_process.id);
    exit (EXIT_SUCCESS);
}
