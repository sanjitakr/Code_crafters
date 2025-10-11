CC = gcc
CFLAGS = `pkg-config --cflags gtk4` -pthread
LIBS = `pkg-config --libs gtk4` -pthread


all: server client


server: server.c
   $(CC) server.c -o server -pthread


client: client.c
   $(CC) client.c -o client $(CFLAGS) $(LIBS)


clean:
   rm -f server client
