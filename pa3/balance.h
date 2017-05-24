#ifndef _BALANCE_H_
#define _BALANCE_H_

#include "proc.h"
#include "messaging.h"

void add_balance_state_to_history ( BalanceHistory* history, BalanceState state );
void commit_transaction ( proc_t* proc, Message* msg );
#endif // _BALANCE_H_
