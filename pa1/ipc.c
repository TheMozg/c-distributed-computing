#include "ipc.h"
#include "proc.h"
#include <unistd.h>
#include <stdio.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE -1

int send(void * self, local_id dst, const Message * msg) {
    proc_t * proc = self;
    return ( write(proc->fd_writ[dst], msg, sizeof(Message)) < 0 ) ? EXIT_SUCCESS : EXIT_FAILURE;
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
    return -1;
}

int receive_any(void * self, Message * msg) {
    proc_t * proc = self;
    read(proc->fd_read[proc->id], msg, sizeof(Message));
    return 0;
}
