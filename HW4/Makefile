all: server client

server: server.c util.c client_server_conn.c account_manager.c
	gcc -o server server.c util.c client_server_conn.c account_manager.c

client: client.c util.c client_server_conn.c account_manager.c
	gcc -o client client.c util.c client_server_conn.c account_manager.c