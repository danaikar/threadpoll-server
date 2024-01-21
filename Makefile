CC = gcc
CFLAGS = -Wall -pthread
DEPS = headers.h

poller: poller.o
	$(CC) -o poller poller.o $(CFLAGS) 

pollSwayer: pollSwayer.o
	$(CC) -o pollSwayer pollSwayer.o $(CFLAGS) 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf *.o poller pollSwayer


valgrind-poller: poller.o
	$(CC) -o poller poller.o $(CFLAGS)
	valgrind --leak-check=full ./poller 5634 8 16 pollLog.txt pollStats.txt

valgrind-pollSwayer: pollSwayer.o
	$(CC) -o pollSwayer pollSwayer.o $(CFLAGS)
	valgrind --leak-check=full ./pollSwayer linux01.di.uoa.gr 5634 inputFile.txt