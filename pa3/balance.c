#include <stdio.h>

#include "balance.h"
#include "lamport.h"

void add_balance_state_to_history (BalanceHistory* history, BalanceState state ) {
    if (state.s_time < history->s_history_len - 1){
        fprintf(stderr, "Critial error: cannot update balance state in the past - state time %hd, max time %d.\n", state.s_time, history->s_history_len -1);
        return;
    }
    if (state.s_time >= history->s_history_len - 1 ){
        for( timestamp_t t = history->s_history_len; t < state.s_time; t++ ) {
            if( t == 0 ) {
                fprintf(stderr, "Critial error: balance history does not have initial state");
            }
            history->s_history[t] = history->s_history[t-1];
            history->s_history[t].s_time = t;
        }
        history->s_history[state.s_time] = state;
        history->s_history_len = state.s_time + 1;
    }

}

void add_pending_order_to_history ( BalanceHistory* history, balance_t amount, timestamp_t msg_time ) {
    for( timestamp_t t = msg_time - 1; t < history->s_history_len - 1; t++ ) {
        history->s_history[t].s_balance_pending_in = amount;
    }
}


void commit_transaction ( proc_t* proc, Message* msg ) {
    TransferOrder* trans = (TransferOrder*)msg->s_payload;

    if( trans->s_src == proc->id ) {
        proc->b_state.s_balance -= trans->s_amount;

        add_balance_state_to_history(&(proc->b_history), proc->b_state);

        Message msg_fwd = create_message ( proc, TRANSFER, trans, sizeof(TransferOrder) );

        while (send ( proc, trans->s_dst, &msg_fwd ) == -1);
    }

    if( trans->s_dst == proc->id ) {
        proc->b_state.s_balance += trans->s_amount;
        proc->b_state.s_balance_pending_in = 0;

        add_balance_state_to_history(&(proc->b_history), proc->b_state );
        add_pending_order_to_history(&(proc->b_history), trans->s_amount, msg->s_header.s_local_time );

        send_status ( proc, PARENT_ID, ACK );
    }

}
