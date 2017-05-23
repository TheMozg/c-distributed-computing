#include <stdio.h>
#include <string.h>
#include "logger.h"

Message create_message ( proc_t* proc, MessageType type, void* contents, uint16_t size ) {
    proc->b_state.s_time++;
    Message msg;
    msg.s_header.s_magic = MESSAGE_MAGIC;
    msg.s_header.s_payload_len = 0;
    msg.s_header.s_type = type;
    msg.s_header.s_local_time = proc->b_state.s_time;
    if( contents != NULL ) {
        msg.s_header.s_payload_len = size;
        memcpy(&(msg.s_payload), contents, size);
    }

    return msg;
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
    Message msg = create_message ( proc, status, NULL, 0 );
    if (send_multicast( proc, &msg ) == -1)
        fprintf(stderr, "Critial error: send_multicast\n");
}

void send_status ( proc_t* proc, local_id dst, MessageType status ) {
    Message msg = create_message ( proc, status, NULL, 0 );
    while( send( proc, dst, &msg ) == -1 );
}

