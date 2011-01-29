# $Id: Makefile,v 1.2 2007/05/24 12:21:45 pyr Exp $

TRUEPREFIX?=	/usr/local

BINDIR 	=	${TRUEPREFIX}/bin
MANDIR	=	${TRUEPREFIX}/man/cat
PROG	=	ipcalc
CFLAGS	+=	-Wall -Werror -Wstrict-prototypes -g3

.include <bsd.prog.mk>
