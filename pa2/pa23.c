#include <stdio.h>
#include <stdlib.h>

#include <argp.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#include "banking.h"
#include "ipc.h"
#include "proc.h"
#include "logger.h"

#ifdef _DEBUG_PA_
    #define DEBUG(x) x
#else
    #define DEBUG(x) __asm__("nop")
#endif

Message create_message ( MessageType type, void* contents ) {
    Message msg;

    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = strlen(contents);
    msg.s_header.s_type = type;
    msg.s_header.s_local_time = get_physical_time();
    memcpy(&(msg.s_payload), contents, strlen(contents));

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

    Message msg_snd = create_message ( TRANSFER, &trans );
    
    DEBUG(printf("Sending transaction, src %d\n", src));

    while( send( parent_data, src, &msg_snd ) == -1 );

    Message msg_rcv;

    DEBUG(printf("Waiting ACK, dst %d\n", dst));

    while( msg_rcv.s_header.s_type != ACK ) { // Waiting for ACK from dst
        receive( parent_data, dst, &msg_rcv );
    }

    DEBUG(printf("TRANSACTION DONE src %d dst %d\n", src, dst));
}

void wait_for_all_messages ( proc_t* proc, MessageType status ) {

    int counter = 0;

    int procs_to_wait = proc->process_count - 2; // Don't wait PARENT process and itself
    if (proc->id == PARENT_ID) procs_to_wait++;
    
    do {
        Message msg;
        receive_any( proc, &msg );
        if ( msg.s_header.s_type == status ) counter++;
    } while ( counter < procs_to_wait );
}

void send_status_to_all ( proc_t* proc, void* buf, MessageType status ) {
    Message msg = create_message ( status, buf );
    send_multicast( proc, &msg );
}

void send_status ( proc_t* proc, local_id dst, void* buf, MessageType status ) {
    Message msg = create_message ( status, buf );
    send( proc, dst, &msg );
}


void children_routine ( proc_t* proc, char* buf ) {
    
    // Starting routine
    send_status ( proc, PARENT_ID, buf, STARTED );

    // wait_for_all_messages ( proc, STARTED );

    // log_received_all_started ( proc );

    // Main routine
    while(1) {

        Message msg;
        do {
            receive_any( proc, &msg );
            
            DEBUG(printf("Received status %d id %d\n", msg.s_header.s_type, proc->id));

        } while ( msg.s_header.s_type != TRANSFER && msg.s_header.s_type != STOP );

        MessageType status = msg.s_header.s_type;
        TransferOrder *trans;

        switch( status ) {
            case TRANSFER:
                trans = (TransferOrder*) msg.s_payload;
                if( trans->s_src == proc->id ) {
                    DEBUG(printf("Received TRANSFER src id %d\n", proc->id));
                    proc->balance_state.s_balance -= trans->s_amount;
                    proc->balance_state.s_time = get_physical_time();
                    log_transfer_out( trans );

                    Message msg = create_message( TRANSFER, trans );

                    send ( proc, trans->s_dst, &msg );
                }

                if( trans->s_dst == proc->id ) {
                    DEBUG(printf("Received TRANSFER dst id %d\n", proc->id));
                    proc->balance_state.s_balance -= trans->s_amount;
                    proc->balance_state.s_time = get_physical_time();
                    log_transfer_in( trans );

                    send_status ( proc, PARENT_ID, buf, ACK );
                }

                break;
            case STOP:
                send_status ( proc, PARENT_ID, buf, DONE );
                char* buf2 = log_done ( proc );
                free (buf2);
                return;
                break;
            default:
                break;
        }
    }
    
    // Ending routine

    //send_status_to_all ( proc, buf, DONE );

    
    //wait_for_all_messages ( proc, DONE );
    
    //log_received_all_done ( proc );
}

void parent_routine ( proc_t* proc ) {
    wait_for_all_messages ( proc, STARTED );

    log_received_all_started ( proc );

    // Do robbery after all STARTED messages received
    bank_robbery( proc, proc->process_count - 1 );
    DEBUG(printf("--BANK IS ROBBED--\n"));

    char* buf = "blabla";
    send_status_to_all ( proc, buf, STOP );

    wait_for_all_messages ( proc, DONE );
    log_received_all_done ( proc );

    while( wait(NULL) > 0); // Wait for all children to stop
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

int main(int argc, char * argv[])
{
    //print_history(all);

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
