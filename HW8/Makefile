all: server client

server: server.c util.c
	gcc -o server server.c util.c -lpthread

client: client.c util.c
	gcc -o client client.c util.c