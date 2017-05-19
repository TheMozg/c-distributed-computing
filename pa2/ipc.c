#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>

#include "ipc.h"
#include "proc.h"

/*
 * O_NONBLOCK enabled, n <= PIPE_BUF (n in theory is always less 
 * than PIPE_BUF because PIPE_BUF is 16 pages, so our msg struct
 * should fit).
 * If there is room to write n bytes to the pipe, then write(2)
 * succeeds immediately, writing all n bytes; otherwise write(2)
 * fails, with errno set to EAGAIN.
 */

int send(void * self, local_id dst, const Message * msg) {
    proc_t * proc = self;
    int status = write(proc->fd_writ[proc->id][dst], msg, sizeof(MessageHeader) + msg->s_header.s_payload_len);
    
    /*if (status == -1) {
        printf("dead %s\n", strerror(errno));
        return -1;
    }*/

    return (status != -1) ? 0 : -1;
}

int send_multicast(void * self, const Message * msg) {
    proc_t * proc = self;

    for ( local_id i = 0; i < proc->process_count; i++ ) {
        if ( i != proc->id )
            send( proc, i, msg );
    }

    return 0;
}

int receive(void * self, local_id from, Message * msg) {
    proc_t * proc = self;

    while( read(proc->fd_read[proc->id][from], NULL, 0) == -1 ); // Checking for errors

    int status = read(proc->fd_read[proc->id][from], msg, sizeof(Message));

    /*if (status == -1) {
        printf("deadread %s\n", strerror(errno));
        return -1;
    }*/

    return (status != -1) ? 0 : -1;
}

// TODO: Test
/*
 * Doing receive cycling through ids until something is read
 */
int receive_any(void * self, Message * msg) {
    proc_t * proc = self;
    while(1) {
       for ( local_id from = 0; from < proc->process_count; from++ ) {
            while( read(proc->fd_read[proc->id][from], NULL, 0) == -1 ); // Checking for errors
            int res = read(proc->fd_read[proc->id][from], msg, sizeof(Message));
            if ( res != -1 ) return 0;
            if ( from == proc->process_count - 1 ) from = 0;
       } 
    }
    return 0;
}
