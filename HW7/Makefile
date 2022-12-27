all: server client

server: server.c account_manager.c util.c
	gcc -o server server.c account_manager.c util.c

client: client.c account_manager.c util.c
	gcc -o client client.c account_manager.c util.c