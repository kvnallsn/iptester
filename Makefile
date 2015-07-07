C       = gcc
CFLAGS  = -Wall
LDFLAGS = 
CLIENT_SRC = client.o ip.o
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_SRC = server.o ip.o
SERVER_OBJ = $(SERVER_SRC:.c=.o)

all: client server

client: $(CLIENT_OBJ)
	$(CC) $(LDFLAGS) $(CLIENT_OBJ) -o $@

server: $(SERVER_OBJ)
	$(CC) $(LDFLAGS) $(SERVER_OBJ) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

tar:
	tar -c client.c Makefile > pkg.tar

clean:
	rm client client.o

.PHONY: clean
