
OBJS=	funcs.h main.c funcs.c 
GCC=	gcc
PROG= 	main

all:	${PROG}


clear:
	rm -f ${OBJS} *~ ${PROG}

${PROG}:	${OBJS}
			${GCC} -o ${PROG} ${OBJS}

.c.o:
			${GCC} -c $< -c -o $@

make: 	${OBJS}
	${GCC} ${OBJS} -O ${PROG}
	