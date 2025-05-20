BIN	:= bin
TARGET	:= ve
CC	:= gcc
C_FLAGS	:= -Wall -Wextra
H_FILES	:= $(shell find src -name '*.h')
C_FILES	:= $(shell find src -name '*.c')

all: ${C_FILES} ${H_FILES}
	mkdir -p ${BIN}
	${CC} -o ${BIN}/${TARGET} ${C_FLAGS} ${C_FILES}

run: all
	valgrind --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--verbose ${BIN}/${TARGET}

.PHONY:	clean
clean:
	rm -rf ${BIN}

