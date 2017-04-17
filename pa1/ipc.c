#include <unistd.h>
#include <stdio.h>

#include "ipc.h"
#include "proc.h"

int send(void * self, local_id dst, const Message * msg) {
    proc_t * proc = self;
    return ( write(proc->fd_writ[dst], msg, sizeof(Message)) < 0 ) ? 0 : -1;
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
    while ( read(proc->fd_read[from], msg, sizeof(Message)) < 0);
    return 0;
}

int receive_any(void * self, Message * msg) {
    proc_t * proc = self;
    read(proc->fd_read[proc->id], msg, sizeof(Message));
    return 0;
}
