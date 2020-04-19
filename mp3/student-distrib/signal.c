/** signal.c -- functions used to control signals */

#include "signal.h"
#include "process.h"

#define SUCCESS 0
#define FAIL   -1


/* functions used to construct signal handlers */
#define SIGHAND_SET(sighand_name, sig_num, default_action)    \
void sighand_name(void){\
    default_action(sig_num);                                                                        \
}   

/* Default Action -- Kill the task */
void _sig_kill_curr_task(int32_t sig_num){
    process_terminate(sig_num + 40);
}

/* Default Action -- Ignored */
void _sig_ignored(int32_t sig_num){
    // do nothing
}

/* construct five signal handlers for initialization */
SIGHAND_SET(sig_hdlr_div_zero,  0, _sig_kill_curr_task);
SIGHAND_SET(sig_hdlr_segfault,  1, _sig_kill_curr_task);
SIGHAND_SET(sig_hdlr_interrupt, 2, _sig_kill_curr_task);
SIGHAND_SET(sig_hdlr_alarm,     3, _sig_ignored);
SIGHAND_SET(sig_hdlr_user1,     4, _sig_ignored);

/* initialize sighand info in process_t */
void init_sighand(sighand_t* sighand){
    sighand->sig_handler[0] = (void*) sig_hdlr_div_zero;
    sighand->sig_handler[1] = (void*) sig_hdlr_segfault;
    sighand->sig_handler[2] = (void*) sig_hdlr_interrupt;
    sighand->sig_handler[3] = (void*) sig_hdlr_alarm;
    sighand->sig_handler[4] = (void*) sig_hdlr_user1;
}


/* set up signal handler acoording to input info */
int32_t set_sig_hdlr(sighand_t* sighand, int32_t sig_num, void* sig_hdlr){
    sighand->sig_handler[sig_num] = sig_hdlr;
    return SUCCESS;
}

/* send signal to correpsonding process */
int32_t send_signal(int32_t sig_num, int32_t pid){

    /* sanity check */
    if(sig_num < 0 || sig_num >= MAX_SIG_NUM) return FAIL;

    /* set signal into pending array */
    process_t* to = get_process_ptr(pid);
    if(!to->sigpending[sig_num]){
        to->sigpending[sig_num] = 1;
    }

    /* return 0 for success */
    return SUCCESS;
}



void _handle_signal(int32_t sig_num, sighand_t* sighand){
    /**
     * 
     *  need to be done!!!
     * 
     * 
     */
    // sighand->sig_handler[sig_num]();

}


int32_t handle_pengding_signal(int32_t pid){

    /* get pointer of process to be checked */
    process_t* p = get_process_ptr(pid);

    /* check signal pending array */
    int32_t i = 0;
    for(i=0; i < MAX_SIG_NUM; i++){
        if(p->sigpending[i]){
            /* handling one signal */
            _handle_signal(i, &p->sighand);
            p->sigpending[i] = 0;
        }
    }

    /* return */
    return SUCCESS;
}



