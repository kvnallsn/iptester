C       = gcc
CFLAGS  = -Wall
LDFLAGS = -lpcap
CLIENT_SRC = client.o ip.o
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)

all: client

client: $(CLIENT_OBJ)
	$(CC) $(LDFLAGS) $(CLIENT_OBJ) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

tar:
	tar -c client.c Makefile > pkg.tar

clean:
	rm client client.o

.PHONY: clean
