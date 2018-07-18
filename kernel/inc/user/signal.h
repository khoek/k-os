#ifndef KERNEL_USER_SIGNAL_H
#define KERNEL_USER_SIGNAL_H

#include <common/types.h>

typedef unsigned long sigset_t;

#define SIG_SETMASK 0	/* set mask with sigprocmask() */
#define SIG_BLOCK 1	/* set of signals to block */
#define SIG_UNBLOCK 2	/* set of signals to, well, unblock */

/* sigev_notify values
   NOTE: P1003.1c/D10, p. 34 adds SIGEV_THREAD.  */

#define SIGEV_NONE   1  /* No asynchronous notification shall be delivered */
                        /*   when the event of interest occurs. */
#define SIGEV_SIGNAL 2  /* A queued signal, with an application defined */
                        /*  value, shall be delivered when the event of */
                        /*  interest occurs. */
#define SIGEV_THREAD 3  /* A notification function shall be called to */
                        /*   perform notification. */

/*  Signal Generation and Delivery, P1003.1b-1993, p. 63
    NOTE: P1003.1c/D10, p. 34 adds sigev_notify_function and
          sigev_notify_attributes to the sigevent structure.  */

union sigval {
    int    sival_int;    /* Integer signal value */
    void  *sival_ptr;    /* Pointer signal value */
};

struct sigevent {
    int              sigev_notify;               /* Notification type */
    int              sigev_signo;                /* Signal number */
    union sigval     sigev_value;                /* Signal value */
    // void           (*sigev_notify_function)( union sigval );
    //                                              /* Notification function */
    // pthread_attr_t  *sigev_notify_attributes;    /* Notification Attributes */
};

/* Signal Actions, P1003.1b-1993, p. 64 */
/* si_code values, p. 66 */

#define SI_USER    1    /* Sent by a user. kill(), abort(), etc */
#define SI_QUEUE   2    /* Sent by sigqueue() */
#define SI_TIMER   3    /* Sent by expiration of a timer_settime() timer */
#define SI_ASYNCIO 4    /* Indicates completion of asycnhronous IO */
#define SI_MESGQ   5    /* Indicates arrival of a message at an empty queue */

typedef struct {
    int          si_signo;    /* Signal number */
    int          si_code;     /* Cause of the signal */
    union sigval si_value;    /* Signal value */
} siginfo_t;

/*  3.3.8 Synchronously Accept a Signal, P1003.1b-1993, p. 76 */

#define SA_NOCLDSTOP 0x1   /* Do not generate SIGCHLD when children stop */
#define SA_SIGINFO   0x2   /* Invoke the signal catching function with */
                           /*   three arguments instead of one. */
#define SA_ONSTACK   0x4   /* Signal delivery will be on a separate stack. */
#define SA_RESTART   0x8

/* struct sigaction notes from POSIX:
 *
 *  (1) Routines stored in sa_handler should take a single int as
 *      their argument although the POSIX standard does not require this.
 *      This is not longer true since at least POSIX.1-2008
 *  (2) The fields sa_handler and sa_sigaction may overlap, and a conforming
 *      application should not use both simultaneously.
 */

typedef void (*sighandler_t)(int);
typedef void (*siginfohandler_t)(int, siginfo_t *, void *);

#define SIG_DFL ((sighandler_t) 0)	/* Default action */
#define SIG_IGN ((sighandler_t) 1)	/* Ignore action */
#define SIG_ERR ((sighandler_t) -1)	/* Error return */

struct sigaction {
    int         sa_flags;       /* Special flags to affect behavior of signal */
    sigset_t    sa_mask;        /* Additional set of signals to be blocked */
                                /*   during execution of signal-catching */
                                /*   function. */
    union {
    sighandler_t _handler;  /* SIG_DFL, SIG_IGN, or pointer to a function */
    siginfohandler_t _sigaction;
    } _signal_handlers;
};

#define sa_handler    _signal_handlers._handler
#define sa_sigaction  _signal_handlers._sigaction

#define SIGACTION_DFL ((struct sigaction) {.sa_flags = 0, .sa_mask = 0, .sa_handler = SIG_DFL})

#define	MINSIGSTKSZ	2048
#define	SIGSTKSZ	8192

#define	SS_ONSTACK	0x1
#define	SS_DISABLE	0x2

typedef struct sigaltstack {
    void     *ss_sp;    /* Stack base or pointer.  */
    int       ss_flags; /* Flags.  */
    size_t    ss_size;  /* Stack size.  */
} stack_t;

#define sigaddset(what,sig) (*(what) |= (1<<(sig)), 0)
#define sigdelset(what,sig) (*(what) &= ~(1<<(sig)), 0)
#define sigemptyset(what)   (*(what) = 0, 0)
#define sigfillset(what)    (*(what) = ~(0), 0)
#define sigismember(what,sig) (((*(what)) & (1<<(sig))) != 0)
#define	SIGHUP	1	/* hangup */
#define	SIGINT	2	/* interrupt */
#define	SIGQUIT	3	/* quit */
#define	SIGILL	4	/* illegal instruction (not reset when caught) */
#define	SIGTRAP	5	/* trace trap (not reset when caught) */
#define	SIGIOT	6	/* IOT instruction */
#define	SIGABRT 6	/* used by abort, replace SIGIOT in the future */
#define	SIGEMT	7	/* EMT instruction */
#define	SIGFPE	8	/* floating point exception */
#define	SIGKILL	9	/* kill (cannot be caught or ignored) */
#define	SIGBUS	10	/* bus error */
#define	SIGSEGV	11	/* segmentation violation */
#define	SIGSYS	12	/* bad argument to system call */
#define	SIGPIPE	13	/* write on a pipe with no one to read it */
#define	SIGALRM	14	/* alarm clock */
#define	SIGTERM	15	/* software termination signal from kill */
#define	SIGURG	16	/* urgent condition on IO channel */
#define	SIGSTOP	17	/* sendable stop signal not from tty */
#define	SIGTSTP	18	/* stop signal from tty */
#define	SIGCONT	19	/* continue a stopped process */
#define	SIGCHLD	20	/* to parent on child stop or exit */
#define	SIGCLD	20	/* System V name for SIGCHLD */
#define	SIGTTIN	21	/* to readers pgrp upon background tty read */
#define	SIGTTOU	22	/* like TTIN for output if (tp->t_local&LTOSTOP) */
#define	SIGIO	23	/* input/output possible signal */
#define	SIGPOLL	SIGIO	/* System V name for SIGIO */
#define	SIGWINCH 24	/* window changed */
#define	SIGUSR1 25	/* user defined signal 1 */
#define	SIGUSR2 26	/* user defined signal 2 */

/* Real-Time Signals Range, P1003.1b-1993, p. 61
   NOTE: By P1003.1b-1993, this should be at least RTSIG_MAX
         (which is a minimum of 8) signals.
 */
#define SIGRTMIN 27
#define SIGRTMAX 31

#define NSIG	32      /* signal 0 implied */

#endif
