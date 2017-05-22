#ifndef _MESSAGING_H_
#define _MESSAGING_H_

#include "ipc.h"
#include "proc.h"
#include "messaging.h"

Message create_message ( MessageType type, void* contents, uint16_t size );

void wait_for_all_messages ( proc_t* proc, MessageType status );

void send_status_to_all ( proc_t* proc, MessageType status );

void send_status ( proc_t* proc, local_id dst, MessageType status );

#endif // _MESSAGING_H_

