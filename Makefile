CC = gcc
CFLAGS = -std=c99 -Wall -O2

all: main1 main2 main3

main1: main1.c
	$(CC) $(CFLAGS) -o main1 main1.c

main2: main2.c
	$(CC) $(CFLAGS) -o main2 main2.c

main3: main3.c
	$(CC) $(CFLAGS) -o main3 main3.c

clean:
	rm -f main1 main2 main3 out1.txt out2.txt out3.txt
