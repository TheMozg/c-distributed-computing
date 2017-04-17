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

void children_routine () {
    
    // Wait for STARTED messages from all other processes
  /*  for (local_id i = 0; i < process_count; i++) {
        if(i != this_process.id && (i != PARENT_ID || this_process.id == PARENT_ID)){
            Message msg;
            receive(&this_process, i, &msg);
            //char text[MAX_PAYLOAD_LEN];
            //memcpy(text, msg.s_payload, msg.s_header.s_payload_len);
            //log_output(fd_event, text);
        }
    }*/
}

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

    // Send messages to all other processes
    if (this_process.id != PARENT_ID){
        Message msg = create_message ( STARTED, buf );
        send_multicast(&this_process, &msg);
    }
    
    free(buf);

    if (this_process.id == PARENT_ID) goto end;

    // Wait for STARTED messages from all other processes. Return value is number of DONE messages.
    int counter_done = wait_for_all_started ( &this_process );

    if (this_process.id != PARENT_ID){
        Message msg = create_message ( DONE, buf );
        send_multicast(&this_process, &msg);
        log_received_all_started ( this_process.id );
    }
    
    wait_for_all_done ( &this_process, counter_done );

    if (this_process.id != PARENT_ID){
        Message msg = create_message ( DONE, buf );
        send_multicast(&this_process, &msg);
        log_received_all_done ( this_process.id );
        log_done ( this_process.id );
    }
    
end:
    // Wait for children
    if (this_process.id == PARENT_ID){
        int counter_done = wait_for_all_started ( &this_process );
        wait_for_all_done ( &this_process, counter_done );
    }

    // Exit
    log_output(fd_event, "P %d quit\n", this_process.id);
    if (this_process.id == PARENT_ID) close_log();
    exit (EXIT_SUCCESS);
}
