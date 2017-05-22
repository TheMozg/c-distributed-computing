#ifdef _DEBUG_IPC_
    #define _GNU_SOURCE
    #include <string.h>
#endif

#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include "ipc.h"
#include "proc.h"

#ifdef _DEBUG_IPC_
    #define FAIL_SLEEP 10000
    #define DEBUG(x) x
#else
    #define DEBUG(x) __asm__("nop")
#endif

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
    int status = write(proc->fd_writ[proc->id][dst], (char *)msg, sizeof(MessageHeader) +
                                msg->s_header.s_payload_len);

    #ifdef _DEBUG_IPC_
    printf("Sending from %d to %d\n", proc->id, dst);
    if (status == -1) {
        printf("Failed to write, id %d: %s\n", proc->id, strerror(errno));
        usleep(FAIL_SLEEP);
        return -1;
    }
    if (status < sizeof(MessageHeader) + msg->s_header.s_payload_len) {

    printf("DIDNT WRITE ENOUGH BYTES, id %d\n", proc->id);
    return -1;
    }
    #endif

    return (status != -1) ? 0 : -1;
}

int send_multicast(void * self, const Message * msg) {
    proc_t * proc = self;

    for ( local_id i = 0; i < proc->process_count; i++ ) {
        if ( i != proc->id )
            if ( send( proc, i, msg ) == -1 )
                return -1;
    }

    return 0;
}

int receive(void * self, local_id from, Message * msg) {
    proc_t * proc = self;

    // Checks for errors from read. Mostly to avoid EBADF.
    while( read(proc->fd_read[proc->id][from], NULL, 0) == -1 );

   /*
    * EAGAIN
    * The file descriptor fd refers to a file other 
    * than a socket and has been marked nonblocking 
    * (O_NONBLOCK), and the read would block.
    */

    while ( read(proc->fd_read[proc->id][from], &(msg->s_header), sizeof(MessageHeader)) == -1 ) {
       if (errno != EAGAIN) {
            DEBUG(fprintf(stderr, "Crit err: partial receive (message header) - expected %lu bytes, got %d.\n",
                sizeof(MessageHeader), status));
            return -1;
        }
    }

    int payload_len = msg->s_header.s_payload_len;
    while (read(proc->fd_read[proc->id][from], msg->s_payload, payload_len) != payload_len) {

        DEBUG(fprintf(stderr, "Crit err: partial receive (message payload) - expected %hu bytes, got %d.\n",
            msg->s_header.s_payload_len, status));
        return -1;
    }

    return 0;
}

/*
 * Doing read cycling through ids until something is read.
 * Do not receive messages from process itself.
 */
int receive_any(void * self, Message * msg) {
    proc_t *proc = self;
    while (1) {
        for ( local_id from = 0; from < proc->process_count; from++ ) {
            if ( from != proc->id ) {
                if ( read( proc->fd_read[proc->id][from], &(msg->s_header), sizeof(Message) ) > 0 ) {
                    if ( msg->s_header.s_payload_len > 0 ) {
                        read( proc->fd_read[proc->id][from], (char *)&(msg->s_payload), 
                                msg->s_header.s_payload_len );
                    }
                    return 0;
                }
            }
        }
    }
}
