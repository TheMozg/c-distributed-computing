#include <unistd.h>
#include <stdio.h>

#include "ipc.h"
#include "proc.h"

int send(void * self, local_id dst, const Message * msg) {
    proc_t * proc = self;
    return ( write(proc->fd_writ[proc->id][dst], msg, sizeof(Message)) < 0 ) ? 0 : -1;
}

int send_multicast(void * self, const Message * msg) {
    proc_t * proc = self;

    for ( local_id i = 0; i < proc->process_count; i++ ) {
        if ( i != proc->id )
            send ( proc, i, msg );
    }

    return 0;
}

int receive(void * self, local_id from, Message * msg) {
    proc_t * proc = self;
    read(proc->fd_read[proc->id][from], msg, sizeof(Message));
    return 0;
}

int receive_any(void * self, Message * msg) {
    //proc_t * proc = self;
    //read(proc->fd_read[proc->id], msg, sizeof(Message));
    return 0;
}
