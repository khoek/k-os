/* libc/sys/linux/sys/termios.h - Terminal control definitions */

/* Written 2000 by Werner Almesberger */


#ifndef _SYS_TERMIOS_H
#define _SYS_TERMIOS_H

#include <sys/types.h>

#define __MAX_BAUD B4000000

typedef unsigned char	cc_t;
typedef unsigned int	speed_t;
typedef unsigned int	tcflag_t;

#define NCCS 19
struct termios {
	tcflag_t c_iflag;		/* input mode flags */
	tcflag_t c_oflag;		/* output mode flags */
	tcflag_t c_cflag;		/* control mode flags */
	tcflag_t c_lflag;		/* local mode flags */
	cc_t c_line;			/* line discipline */
	cc_t c_cc[NCCS];		/* control characters */
};

#define ECHO	0000010

#define	TCSAFLUSH	2

#define	RTSXOFF		0x0001		/* RTS flow control on input */
#define	CTSXON		0x0002		/* CTS flow control on output */
#define	DTRXOFF		0x0004		/* DTR flow control on input */
#define DSRXON		0x0008		/* DCD flow control on output */

/* grr, this shouldn't have to be here */

int tcgetattr(int fd,struct termios *termios_p);
int tcsetattr(int fd,int optional_actions,const struct termios *termios_p);

#endif
