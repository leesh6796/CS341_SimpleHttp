CC = gcc
CLIENT_OBJS = client_main.o tools.o
CLIENT_TARGET = client

SERVER_OBJS = server_main.o tools.o
SERVER_TARGET = server

TESTER_OBJS = tester.o
TESTER_TARGET = test

all: $(CLIENT_TARGET) $(SERVER_TARGET) $(TESTER_TARGET)

clean:
	rm -f *.o
	rm -f $(CLIENT_TARGET)

$(CLIENT_TARGET) : $(CLIENT_OBJS)
	$(CC) -o $@ $(CLIENT_OBJS)

$(SERVER_TARGET) : $(SERVER_OBJS)
	$(CC) -o $@ $(SERVER_OBJS)

$(TESTER_TARGET) : $(TESTER_OBJS)
	$(CC) -o $@ $(TESTER_OBJS)

client_main.o : client_main.h tools.h client_main.c
server_main.o : server_main.h tools.h server_main.c
tools.o : tools.h tools.c

tester.o : tester.c