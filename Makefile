CC = gcc
CFLAGS = -Wall -Wextra -g
IN = lexer.c hashtable.c main.c
OUT = out

make: $(IN)
	$(CC) $(IN) -o $(OUT) $(CFLAGS)
	./$(OUT)