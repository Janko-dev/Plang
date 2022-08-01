CC = gcc
CFLAGS = -Wall -Wextra -Wno-unknown-pragmas -g -O0 -pedantic -std=c99
IN = tokenizer.c main.c
OUT = plang

make: $(IN)
	$(CC) $(IN) -o $(OUT) $(CFLAGS)