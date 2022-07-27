CC = gcc
CFLAGS = -Wall -Wextra -Wno-unknown-pragmas -g -std=c99
IN = lexer.c parser.c interpreter.c main.c
OUT = plang

make: $(IN)
	$(CC) $(IN) -o $(OUT) $(CFLAGS)