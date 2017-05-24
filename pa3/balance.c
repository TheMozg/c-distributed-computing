#include <stdio.h>

#include "balance.h"
#include "lamport.h"

void add_balance_state_to_history (BalanceHistory* history, BalanceState state, timestamp_t fill_time ) {
    if (state.s_time < history->s_history_len - 1){
        fprintf(stderr, "Critial error: cannot update balance state in the past - state time %hd, max time %d.\n", state.s_time, history->s_history_len -1);
        return;
    }

    printf("Fill time at id %d: %d %d %d %d\n", history->s_id, history->s_history_len, fill_time, get_lamport_time(), state.s_time );
    if (state.s_time >= history->s_history_len - 1 ){
        for( timestamp_t t = history->s_history_len; t < state.s_time; t++ ) {
            if( t == 0 ) {
                fprintf(stderr, "Critial error: balance history does not have initial state");
            }
            //history->s_history[t] = history->s_history[t-1];
            history->s_history[t].s_time = t;
            history->s_history[t].s_balance_pending_in = history->s_history[t-1].s_balance_pending_in;
            history->s_history[t].s_balance = history->s_history[t-1].s_balance;
        }
        for( timestamp_t t = fill_time; t < state.s_time; t++ ) {
            printf("BEPIS\n");
         //   history->s_history[t] = history->s_history[fill_time];
         //   history->s_history[t].s_time = t;
            //history->s_history[t] = history->s_history[t-1];

            //history->s_history[t].s_balance = state.s_balance;
            //history->s_history[t].s_balance_pending_in = state.s_balance_pending_in;
            //history->s_history[t].s_time = t;
            history->s_history[t].s_balance_pending_in = 0;
        }

        history->s_history[state.s_time] = state;
        history->s_history_len = state.s_time + 1;
        /*for( timestamp_t t = history->s_history_len; t < state.s_time; t++ ) {
            history->s_history[t] = history->s_history[t-1];
            history->s_history[t].s_time = t;
            if( t > fill_time && t <= get_lamport_time() ) {
                history->s_history[t].s_balance_pending_in = history->s_history[fill_time - 1].s_balance_pending_in;
            } else {
                history->s_history[t].s_balance_pending_in = 0;
            }
        }*/
    }
/*
    for( timestamp_t t = fill_time + 1; t < get_lamport_time(); t++ ) {
        history->s_history[t].s_balance_pending_in = history->s_history[fill_time].s_balance_pending_in;
    }
*/


}

void commit_transaction ( proc_t* proc, Message* msg ) {
    TransferOrder* trans = (TransferOrder*)msg->s_payload;

    if( trans->s_src == proc->id ) {
        proc->b_state.s_balance -= trans->s_amount;
        proc->b_state.s_balance_pending_in = trans->s_amount;

        add_balance_state_to_history(&(proc->b_history), proc->b_state, msg->s_header.s_local_time );

        Message msg_fwd = create_message ( proc, TRANSFER, trans, sizeof(TransferOrder) );

        while (send ( proc, trans->s_dst, &msg_fwd ) == -1);
    }

    if( trans->s_dst == proc->id ) {
        proc->b_state.s_balance += trans->s_amount;
        //proc->b_state.s_balance_pending_in = 0;

        send_status ( proc, PARENT_ID, ACK );

        add_balance_state_to_history(&(proc->b_history), proc->b_state, msg->s_header.s_local_time );
    }

}
