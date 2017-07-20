FLAGS=-g -D_REENTRANT
HEADERS=network.h server.h requests.h bbuffer.h
LIBS=-lpthread -lrt
CC=gcc

myserver: myserver.o network.o bbuffer.o
	$(CC) -o myserver myserver.o network.o bbuffer.o $(LIBS) $(FLAGS)

myserver.o: myserver.c $(HEADERS)
	$(CC) $(FLAGS) -c myserver.c

network.o: network.c $(HEADERS)
	$(CC) $(FLAGS) -c network.c

bbuffer.o: bbuffer.c $(HEADERS)
	$(CC) $(FLAGS) -c bbuffer.c

all: myserver

clean:
	-rm network.o myserver.o bbuffer.o myserver
