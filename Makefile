C       = gcc
CFLAGS  = -Wall
LDFLAGS = 
SRC = iptester.c ip.c
OBJ = $(SRC:.c=.o)

all: iptester

iptester: $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

tar:
	tar -c iptester.c Makefile > pkg.tar

clean:
	rm iptester iptester.c

.PHONY: clean
