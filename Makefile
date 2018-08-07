# Project: SUMMER LANGUAGE

CPP  = g++
CC   = gcc
OBJ  = obj/vm.o obj/mini.o
BIN  = mini
CFLAGS = -O2 -Wall -DUSE_VM
RM = rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before mini all-after

clean: clean-custom
	${RM} $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o "mini" $(LIBS)

obj/vm.o: src/vm.c
	$(CC) -c src/vm.c -o obj/vm.o $(CFLAGS)

obj/mini.o: mini.c
	$(CC) -c mini.c -o obj/mini.o $(CFLAGS)
