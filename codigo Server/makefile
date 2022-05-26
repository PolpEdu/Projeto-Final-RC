
OBJS=	funcs.h main.c funcs.c 
OBJSclient = client.c
GCC=	gcc
PROG= 	stock_server
PROGC = operations_terminal

all:	${PROG} ${PROGC}


clear:
	rm -f ${OBJS} *~ ${PROG}
	rm -f ${PROGC}

${PROG}:	${OBJS}
			${GCC} -o ${PROG} ${OBJS}
${PROGC}:	${OBJSclient}
			${GCC} -o ${PROGC} ${OBJSclient}


.c.o:
			${GCC} -c $< -c -o $@

make: 	${OBJS}
	${GCC} ${OBJS} -O ${PROG}
	${GCC} ${OBJSclient} -O ${PROGC}
	