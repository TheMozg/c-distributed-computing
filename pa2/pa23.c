#ifdef _DEBUG_PA_
#define _GNU_SOURCE
#endif
#include <stdlib.h>

#include <argp.h>
#include <sys/wait.h>
#include <string.h>

#include "banking.h"
#include "ipc.h"
#include "proc.h"
#include "logger.h"

#ifdef _DEBUG_PA_
    #include <stdio.h>
    #include <unistd.h>
    #define DEBUG(x) x
#else
    #define DEBUG(x) __asm__("nop")
#endif

Message create_message ( MessageType type, void* contents, uint16_t size ) {
    Message msg;

    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = 0;
    msg.s_header.s_type = type;
    msg.s_header.s_local_time = get_physical_time();
    if( contents != NULL ) {
        msg.s_header.s_payload_len = size;
        memcpy(&(msg.s_payload), contents, size);
    }

    return msg;
}

void transfer(void * parent_data, local_id src, local_id dst,
              balance_t amount) {

    proc_t* proc = parent_data;
    if( proc->id != PARENT_ID ) return; // Only parent can transfer

    TransferOrder trans;

    trans.s_src = src;
    trans.s_dst = dst;
    trans.s_amount = amount;

    Message msg_snd = create_message ( TRANSFER, &trans, sizeof(TransferOrder) );
    
    while( send( parent_data, src, &msg_snd ) == -1 );
    log_transfer_out( &trans );

    Message msg_rcv;

    // Waiting for ACK from dst
    do {
        receive( parent_data, dst, &msg_rcv );
    } while ( msg_rcv.s_header.s_type != ACK );
    log_transfer_in( &trans );

}

void wait_for_all_messages ( proc_t* proc, MessageType status ) {

    local_id counter = 0;

    // Don't wait PARENT process and itself
    local_id procs_to_wait;
    if (proc->id == PARENT_ID)
        procs_to_wait = proc->process_count - 1;
    else
        procs_to_wait = proc->process_count - 2;

    while ( counter < procs_to_wait ) {
        Message msg;
        receive_any( proc, &msg );
        if ( msg.s_header.s_type == status ) counter++;
    } 
}

void send_status_to_all ( proc_t* proc, MessageType status ) {
    Message msg = create_message ( status, NULL, 0 );
    send_multicast( proc, &msg );
}

void send_status ( proc_t* proc, local_id dst, MessageType status ) {
    Message msg = create_message ( status, NULL, 0 );
    while( send( proc, dst, &msg ) == -1 );
}

void transaction_snd ( proc_t* proc, TransferOrder* trans, timestamp_t now ) {
    if( trans->s_src == proc->id ) {

        proc->b_state.s_balance -= trans->s_amount;

        proc->b_history.s_history[now] = proc->b_state;
        Message msg = create_message( TRANSFER, trans, sizeof(TransferOrder) );
        
        while (send ( proc, trans->s_dst, &msg ) == -1);
    }
}

void transaction_rcv ( proc_t* proc, TransferOrder* trans, timestamp_t now ) {
	if( trans->s_dst == proc->id ) {

	    proc->b_state.s_balance += trans->s_amount;
	
        proc->b_history.s_history[now] = proc->b_state;

	    send_status ( proc, PARENT_ID, ACK );
	}
}

void commit_transaction ( proc_t* proc, TransferOrder* trans ) {
    timestamp_t now = get_physical_time();
	proc->b_state.s_time = now;

    transaction_snd ( proc, trans, now );
    transaction_rcv ( proc, trans, now );

    proc->b_history.s_history_len++;
}

void children_routine ( proc_t* proc, char* buf ) {
    
    // Starting routine. Phase 1.
    send_status ( proc, PARENT_ID, STARTED );

    /*
     * Counter for phases 2 and 3.
     * When received DONE from all other child processes
     * stop receiving new TRANSFER and STOP messages.
     */
    int done_counter = 0;

    timestamp_t end_time = 0;

    // Main routine
    while( done_counter < proc->process_count - 2 ) { 

        Message msg;

        receive_any( proc, &msg );

        MessageType status = msg.s_header.s_type;

        switch( status ) {
            
            // Phase 2
            case TRANSFER: {
                TransferOrder trans;
                memcpy(&trans, &(msg.s_payload), sizeof(TransferOrder));
                #ifdef _DEBUG_PA_
                printf("\t\t\t\tReceived transfer from %d to %d amount %d\n", trans.s_src, trans.s_dst, trans.s_amount);
                printf("\t\t\t\tPayload length %d\n", msg.s_header.s_payload_len);
                printf("\t\t\t\tSizeof order %lu\n", sizeof(TransferOrder));
                #endif
                commit_transaction( proc, &trans );
                break;
            }

            // Phase 3
            // We use DONE message as a marker that this process finished all transactions
            case STOP:
                send_status_to_all ( proc, DONE ); 
                end_time = get_physical_time();
                DEBUG(printf("id %d ended %d\n", proc->id, end_time));
                break; 
            
            case DONE:
                done_counter++;
                break;

            default:
                break;
        }
    }

    log_done ( proc );

    //Closing balance history by filling between balance changes
    timestamp_t t = 0;

    BalanceState sub = proc->b_history.s_history[t];
    BalanceState temp;

    proc->b_history.s_history[end_time] = proc->b_state;
    for( t = 1 ; t <= end_time; t++ ) {
        temp = proc->b_history.s_history[t];
        if ( temp.s_time == t && sub.s_time != temp.s_time ) {
            sub = temp;
        } else {
            sub.s_time = t;
            proc->b_history.s_history_len++;
            proc->b_history.s_history[t] = sub;
        }
    }
    
    // Sending balance history to parent
    Message b_msg = create_message ( BALANCE_HISTORY, &proc->b_history, 
            (proc->b_history.s_history_len) * sizeof(BalanceState) + 
            sizeof(proc->b_history.s_history_len) + 
            sizeof(proc->b_history.s_id));

    //Message b_msg = create_message ( BALANCE_HISTORY, &proc->b_history, sizeof(BalanceHistory) );
    while( send( proc, PARENT_ID, &b_msg ) == -1 );
}

void parent_routine ( proc_t* proc ) {
    wait_for_all_messages ( proc, STARTED );

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
            history.s_history_len++;
            DEBUG(printf("\tReceived BALANCE_HISTORY from %d\n", temp.s_id));
            history.s_history[temp.s_id - 1] = temp;
            #ifdef _DEBUG_PA_
            for ( local_id i = 0; i < proc->process_count; i++ ) {
                BalanceState tmp = temp.s_history[i];
                printf("\tHISTORY ID %d T %d AM %d len %d\n",
                        temp.s_id, temp.s_history[i].s_time, temp.s_history[i].s_balance, temp.s_history_len);
            }
            #endif
            //memcpy(&history.s_history[temp.s_id - 1], &(msg.s_payload), sizeof(msg.s_payload));
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
        if (args->process_count < 0 || args->process_count > MAX_PROCESS_ID){
            argp_failure(state, 1, 0,"PROCESS_COUNT must be between 0 and %d",MAX_PROCESS_ID);
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
    char * buf = start_procs( &this_process, process_count, balance );

    if (this_process.id != PARENT_ID) {
        children_routine ( &this_process, buf );
    } else parent_routine ( &this_process );

    free(buf);

    return 0;
}
