#ifdef _DEBUG_PA_
#define _GNU_SOURCE
#endif
#include <stdlib.h>

#include <sys/wait.h>
#include <string.h>

#include "banking.h"
#include "ipc.h"
#include "proc.h"
#include "logger.h"
#include "messaging.h"
#include "balance.h"
#include "lamport.h"

/*
argp.h is broken in ubuntu 14.04
it may # define __attribute__(Spec)
workaround is to include it after other headers
*/
#include <argp.h>

#ifdef _DEBUG_PA_
    #include <stdio.h>
    #include <unistd.h>
    #define DEBUG(x) x
#else
    #define DEBUG(x) __asm__("nop")
#endif

void children_routine ( proc_t* proc ) {
    
    // Starting routine. Phase 1.
    send_status( proc, PARENT_ID, STARTED );
    /*
     * Counter for phases 2 and 3.
     * When received DONE from all other child processes
     * stop receiving new TRANSFER and STOP messages.
     */
    int done_counter = 0;

    // Main routine
    while( done_counter < proc->process_count - 2 ) { 

        Message msg;

        receive_any( proc, &msg );

        MessageType status = msg.s_header.s_type;

        switch( status ) {
            
            // Phase 2
            case TRANSFER: {
                #ifdef _DEBUG_PA_
                TransferOrder trans;
                memcpy(&trans, &(msg.s_payload), sizeof(TransferOrder));
                printf("\t\t\t\tReceived transfer from %d to %d amount %d\n", trans.s_src, trans.s_dst, trans.s_amount);
                printf("\t\t\t\tPayload length %d\n", msg.s_header.s_payload_len);
                printf("\t\t\t\tSizeof order %lu\n", sizeof(TransferOrder));
                #endif
                commit_transaction( proc, &msg );
                break;
            }

            // Phase 3
            // We use DONE message as a marker that this process finished all transactions
            case STOP:
                send_status_to_all ( proc, DONE ); 
                break; 
            
            case DONE:
                done_counter++;
                break;

            default:
                break;
        }
    }

    // Close balance
    proc->b_state = proc->b_history.s_history[proc->b_history.s_history_len - 1];
    proc->b_state.s_time = get_lamport_time( );
    proc->b_state.s_balance_pending_in = 0;
    add_balance_state_to_history(&(proc->b_history), proc->b_state, get_lamport_time());

    log_done ( proc );
   
    // Sending balance history to parent
    Message b_msg = create_message ( proc, BALANCE_HISTORY, &proc->b_history, 
            (proc->b_history.s_history_len) * sizeof(BalanceState) + 
            sizeof(proc->b_history.s_history_len) + 
            sizeof(proc->b_history.s_id));

    while( send( proc, PARENT_ID, &b_msg ) == -1 );
}

void parent_routine ( proc_t* proc ) {
    wait_for_all_messages( proc, STARTED );

    log_received_all_started ( proc );

    // Do robbery after all STARTED messages received
    bank_robbery( proc, proc->process_count - 1 );

    DEBUG(printf("\t\t--BANK IS ROBBED--\n"));

    send_status_to_all ( proc, STOP );

    wait_for_all_messages ( proc, DONE );
    log_received_all_done ( proc );

    AllHistory history;
    history.s_history_len = 0;

    while( history.s_history_len < proc->process_count - 1 ) {
        Message msg;
        receive_any( proc, &msg );

        if( msg.s_header.s_type == BALANCE_HISTORY ) {

            BalanceHistory temp;
            memcpy(&temp, &(msg.s_payload), sizeof(msg.s_payload));
            DEBUG(printf("\tReceived BALANCE_HISTORY from %d\n", temp.s_id));
            history.s_history[temp.s_id - 1] = temp;
            history.s_history_len++;

            #ifdef _DEBUG_PA_
            timestamp_t now = get_lamport_time();
            for ( local_id i = 0; i <= now; i++ ) {
                printf("\tHISTORY ID %d T %d AM %d len %d\n",
                        temp.s_id, temp.s_history[i].s_time, temp.s_history[i].s_balance, temp.s_history_len);
            }
            #endif
        }
    }
    DEBUG(printf("\tAllHistory len %d\n", history.s_history_len));

    print_history( &history );
    while( wait(NULL) > 0 ); // Wait for all children to stop
    close_log();
}

static char doc[] = "ITMO Distributed Computing programming assignment #2";

static struct argp_option options[] = {
    {0, 'p', "PROCESS_COUNT", 0, "Number of child processes" },
    {0}
};

typedef struct {
  int balance[MAX_PROCESS_ID];
  int process_count;
} args_t;

static error_t parse_opt (int key, char *arg, struct argp_state *state) {
    args_t *args = state->input;
    switch (key)
    {
    case 'p':
        args->process_count = atoi(arg);
        if (args->process_count < 2 || args->process_count > MAX_PROCESS_ID){
            argp_failure(state, 1, 0,"PROCESS_COUNT must be between 2 and %d",MAX_PROCESS_ID);
        }
        break;

    case ARGP_KEY_ARG:
        if (state->arg_num >= args->process_count){
            argp_usage (state);
        }
        args->balance[state->arg_num] = atoi(arg);
        break;

    case ARGP_KEY_END:
        if (state->arg_num < args->process_count){
            argp_usage (state);
        }
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };

int main(int argc, char * argv[]) {

    args_t args;
    argp_parse (&argp, argc, argv, 0, 0, &args);
    local_id process_count = args.process_count;
    balance_t balance[process_count];

    for (local_id i = 0; i < args.process_count; i++){
        balance[i] = args.balance[i];
    }

    start_log();

    // Increment to get total number of processes
    process_count++;

    proc_t this_process;
 
    // PARENT_ID does not have to be 0
    start_procs( &this_process, process_count, balance );

    if (this_process.id != PARENT_ID) {
        children_routine ( &this_process );
    } else parent_routine ( &this_process );

    return 0;
}
