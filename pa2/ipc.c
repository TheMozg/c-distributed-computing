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
    int status = write(proc->fd_writ[proc->id][dst], msg, sizeof(MessageHeader) + msg->s_header.s_payload_len);
    
    #ifdef _DEBUG_IPC_
    printf("Sending from %d to %d\n", proc->id, dst);
    if (status == -1) {
        printf("Failed to write, id %d: %s\n", proc->id, strerror(errno));
        usleep(FAIL_SLEEP);
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

    int status = read(proc->fd_read[proc->id][from], msg, sizeof(MessageHeader));
    if (status != sizeof(MessageHeader) && status != -1 && status != 0){
        fprintf(stderr, "Critial error: partial receive (message header) - expected %lu bytes, got %d.\n", sizeof(MessageHeader), status);
    }
    if (status == sizeof(MessageHeader) && msg->s_header.s_payload_len != 0){
        int payload_status = read(proc->fd_read[proc->id][from], msg->s_payload, msg->s_header.s_payload_len);
        if (payload_status != msg->s_header.s_payload_len){
            fprintf(stderr, "Critial error: partial receive (message payload) - expected %hu bytes, got %d.\n", msg->s_header.s_payload_len, status);
        }
    }

    #ifdef _DEBUG_IPC_
    if (status == -1) {
        printf("Failed to read from %d, id %d: %s\n", from, proc->id, strerror(errno));
        usleep(FAIL_SLEEP);
        return -1;
    }
    #endif

    return (status == -1 || status == 0) ? -1 : 0;
}

/*
 * Doing read cycling through ids until something is read.
 * Do not receive messages from process itself.
 */
int receive_any(void * self, Message * msg) {
    proc_t * proc = self;
    while(1){
        for ( local_id from = 0; from < proc->process_count; from++ ) {
            if ( from != proc->id ) {
                int status = receive( self, from, msg );
                if ( status != -1 ) return 0;
            }
        } 
    }
}
