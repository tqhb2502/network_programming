/**
 * TCP server
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <pthread.h> /* create thread */
#include <sys/stat.h>

#include "util.h"

#define BUFF_SIZE 2048
#define FILENAME_LEN 20

#define SYS_BYE "#SYS_BYE#"

typedef struct client_connection_t {
    struct sockaddr_in addr;
    int socket;
} client_connection;

void *handle_client(void *param) {

    int connected = 1;

    // get connected socket
    client_connection client = *((client_connection *) param);

    // get client's file name
    char filename[FILENAME_LEN];
    char tmp[FILENAME_LEN];
    memset(filename, '\0', sizeof(filename));
    strcpy(filename, "info/");
    number_to_str(ntohs(client.addr.sin_port), tmp);
    strcat(filename, tmp);
    strcat(filename, ".log");
    printf("%s\n", filename);

    // write message to file
    char buff[BUFF_SIZE];
    int recv_bytes;

    while (connected) {

        FILE *fp = fopen(filename, "a");

        recv_bytes = recv(client.socket, buff, BUFF_SIZE, 0);
        buff[recv_bytes] = '\0';

        if (strcmp(buff, SYS_BYE) == 0) {
            connected = 0;
        } else {
            fwrite(buff, 1, strlen(buff), fp);
        }

        fclose(fp);
    }
    
    // close connected socket
    close(client.socket);
}

int main(int argc, char *argv[]) {

    //* variables declaration
    int port_number; /* opened port */

    int listenfd; /* listen socket descriptor */

    struct sockaddr_in server_addr; /* server address */
    int sin_size; /* size of address structure */

    client_connection client;

    struct stat st; /* for creating "./info" dir if it doesn't exist */

    //* create "./info" dir if not exist
    if (stat("./info", &st) == -1) {
        mkdir("./info", 0777);
    }

    //* init
    if (argc == 2) {
        port_number = str_to_number(argv[1]);
    } else {
        printf("Usage: ./server <port_number>\n");
        exit(0);
    }

    //* create socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        report_error();
        exit(0);
    }

    //* fill server adddress
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port_number);

    //* bind listening socket with server address
    if (bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        report_error();
        exit(0);
    }

    //* set socket for listening to client connection request
    if (listen(listenfd, 10) < 0) {
        report_error();
        exit(0);
    }

    printf("Server started!\n");

    //* communicate with clients
    while (1) {

        sin_size = sizeof(struct sockaddr_in);

        // accept connection request
        if ((client.socket = accept(listenfd, (struct sockaddr *) &client.addr, &sin_size)) < 0) {
            report_error();
            exit(0);
        }

        // create thread to handle client seperately
        pthread_t client_thr;
        if (pthread_create(&client_thr, NULL, handle_client, (void *) &client) < 0) {
            report_error();
            exit(0);
        }
        pthread_detach(client_thr);
    }

    //* close socket
    close(listenfd);

    printf("Server stopped!\n");

    return 0;
}