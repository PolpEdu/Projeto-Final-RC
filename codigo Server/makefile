
OBJS=	funcs.h main.c funcs.c 
FLAGS = -Wall -pthread
OBJSclient = client.c
GCC=	gcc
PROG= 	stock_server
PROGC = operations_terminal

all:	${PROG} ${PROGC}


clear:
	rm -f ${OBJS} *~ ${PROG}
	rm -f ${PROGC}

${PROG}:	${OBJS}
			${GCC} ${FLAGS} -o ${PROG} ${OBJS}
${PROGC}:	${OBJSclient}
			${GCC} ${FLAGS} -o ${PROGC} ${OBJSclient}


.c.o:
			${GCC} -c $< -c -o $@

make: 	${OBJS}
	${GCC} ${FLAGS} ${OBJS} -O ${PROG}
	${GCC} ${FLAGS} ${OBJSclient} -O ${PROGC}
	