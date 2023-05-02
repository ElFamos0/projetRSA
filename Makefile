FILES_C 	=	tcpclient.c\
				list.c
FILES_S   	=	tcpserveur.c \
				command_handler.c \
				list.c

S_PATH	=	srcs/
O_PATH	=	objs/
I_PATH	=	includes/

SRCS_C	=	${addprefix ${S_PATH}, ${FILES_C}}
OBJS_C	=	${addprefix ${O_PATH}, ${FILES_C:.c=.o}}

SRCS_S	=	${addprefix ${S_PATH}, ${FILES_S}}
OBJS_S	=	${addprefix ${O_PATH}, ${FILES_S:.c=.o}}


CC		=	gcc

C_NAME	= tcpclient

S_NAME = tcpserver

RM		=	rm -rf

CFLAGS	= 	-Werror -Wall -O0 -g3 -fno-omit-frame-pointer -fno-optimize-sibling-calls -pthread -lc


${OBJS_C}: ${SRCS_C}
${OBJS_S}: ${SRCS_S}

${O_PATH}%.o:	${S_PATH}%.c 
				@mkdir -p ${dir $@}
				@${CC} ${CFLAGS} -c $< -o $@
				@echo "Compiling"


${C_NAME}:			${OBJS_C}
					@${CC} ${OBJS_C} ${CFLAGS} -o ${C_NAME} -I ${I_PATH}
					@echo "Client compilation is completed !"


${S_NAME}:			${OBJS_S}
					@${CC} ${OBJS_S} ${CFLAGS} -o ${S_NAME} -I ${I_PATH}
					@echo "Server compilation is completed !"


all:		${C_NAME} ${S_NAME}

			
clean:
			@${RM} ${O_PATH}*
			@echo "Removing${S}${S} {O_PATH}${S} "

fclean:		clean
			@${RM} ${C_NAME}
			@echo "Removing${S}${S} ${C_NAME}${S} "
			@${RM} ${S_NAME}
			@echo "Removing${S}${S} ${S_NAME}${S} "

space:
			@echo " "

re:			fclean space all

test:		@mv ftc tests
			@./test_args


.PHONY: all clean fclean re build space