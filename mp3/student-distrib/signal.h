

#ifndef _SIGNAL_H
#define _SIGNAL_H

#include "types.h"


/* maximum signal number */
#define MAX_SIG_NUM 5

/* struct to store pointers of signal handlers */
typedef struct sighand_t{
    void* sig_handler[MAX_SIG_NUM];
} sighand_t;

/* initialize sighand info in process_t */
void init_sighand(sighand_t* sighand);

/* set up signal handler acoording to input info */
int32_t set_sig_hdlr(sighand_t* sighand, int32_t sig_num, void* sig_hdlr);

/* send signal to correpsonding process */
int32_t send_signal(int32_t sig_num, int32_t pid);

/* check and handle current pending signal */
int32_t handle_pengding_signal(int32_t pid) ;


#endif

