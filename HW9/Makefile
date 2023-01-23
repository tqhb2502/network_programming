all: server mcast

server: server.c util.c
	gcc -o server server.c util.c -lpthread

mcast: mcast.c util.c
	gcc -o mcast mcast.c util.c -lpthread