CC = gcc
CFLAGS = -Wall -Wextra -pthread -g

all: server client

server: server.o database.o logger.o
	$(CC) $(CFLAGS) -o server server.o database.o logger.o

client: client.o
	$(CC) $(CFLAGS) -o client client.o

server.o: server.c common.h database.h logger.h
	$(CC) $(CFLAGS) -c server.c

client.o: client.c common.h
	$(CC) $(CFLAGS) -c client.c

database.o: database.c database.h common.h
	$(CC) $(CFLAGS) -c database.c

logger.o: logger.c logger.h
	$(CC) $(CFLAGS) -c logger.c

clean:
	rm -f *.o server client

reset_db:
	rm -f *.dat server.log /tmp/flight_booking.sock
