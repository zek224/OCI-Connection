CC = gcc
CFLAGS = -I../instantclient/sdk/include -L../instantclient -lclntsh -Wl,-rpath=../instantclient
OBJ = server.o database_connection.o

all: server

server: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o server
