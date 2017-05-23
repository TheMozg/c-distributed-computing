#include <stdlib.h>

#include "banking.h"
#include "ipc.h"
#include "proc.h"
#include "logger.h"
#include "messaging.h"

void transfer(void * parent_data, local_id src, local_id dst,
              balance_t amount) {

    proc_t* proc = parent_data;
    if( proc->id != PARENT_ID ) return; // Only parent can transfer

    TransferOrder trans;

    trans.s_src = src;
    trans.s_dst = dst;
    trans.s_amount = amount;

    Message msg_snd = create_message ( proc, TRANSFER, &trans, sizeof(TransferOrder) );
    
    while( send( parent_data, src, &msg_snd ) == -1 );
    log_transfer_out( proc, &trans );

    Message msg_rcv;

    // Waiting for ACK from dst
    do {
        receive( parent_data, dst, &msg_rcv );
    } while ( msg_rcv.s_header.s_type != ACK );

    log_transfer_in( proc, &trans );

}

