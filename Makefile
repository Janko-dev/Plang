CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99
IN = lexer.c hashtable.c parser.c main.c
OUT = out

make: $(IN)
	$(CC) $(IN) -o $(OUT) $(CFLAGS)
	./$(OUT)