/* UDP client */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "util.h"

#define BUFF_SIZE 1024 /* size of buffer */

int main(int argc, char *argv[]) {

    int client_sock; /* client socket file descriptor */
    char buff[BUFF_SIZE], result[BUFF_SIZE];
    struct sockaddr_in server_addr; /* server's address info */
    int bytes_received, bytes_sent, sin_size;
    char server_ip[20]; /* server's IP */
    int port_number; /* server's opened port */

    // init
    printf("Client is running...\n");

    if (argc == 3) {
        strcpy(server_ip, argv[1]);
        port_number = str_to_number(argv[2]);
    } else {
        printf("Usage: ./client server_ip port_number\n");
    }

    // create socket
    if ((client_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error");
        printf("\n");
        exit(0);
    }

    // fill server info
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // send start message to server
    strcpy(buff, "start");
    bytes_sent = sendto(client_sock, buff, strlen(buff), 0,
        (struct sockaddr *) &server_addr, sizeof(struct sockaddr));

    // comunicate with server
    while (1) {

        // input message
        printf("Message: ");
        fgets(buff, sizeof(buff), stdin);
        buff[strlen(buff) - 1] = '\0';
        
        // if input is empty string, send stop message and stop process
        if (strlen(buff) == 0) {
            strcpy(buff, "stop");
            bytes_sent = sendto(client_sock, buff, strlen(buff), 0,
                (struct sockaddr *) &server_addr, sizeof(struct sockaddr));
            printf("Client is stopped\n");
            break;
        }

        // send to server
        bytes_sent = sendto(client_sock, buff, strlen(buff), 0,
            (struct sockaddr *) &server_addr, sizeof(struct sockaddr));

        // receive from server
        bytes_received = recvfrom(client_sock, buff, BUFF_SIZE, 0,
            (struct sockaddr *) &server_addr, &sin_size);
        buff[bytes_received] = '\0';

        // stop if receive stop message
        if (strcmp(buff, "stop") == 0) {
            printf("Client is stopped\n");
            break;
        }

        // received error message
        if (strcmp(buff, "error") == 0) {
            printf("Error\n");
            continue;
        }

        // received ok message
        if (strcmp(buff, "ok") == 0) {

            bytes_received = recvfrom(client_sock, result, BUFF_SIZE, 0,
                (struct sockaddr *) &server_addr, &sin_size);
            result[bytes_received] = '\0';
            if (strlen(result) > 0) {
                printf("%s\n", result);
            }

            bytes_received = recvfrom(client_sock, result, BUFF_SIZE, 0,
                (struct sockaddr *) &server_addr, &sin_size);
            result[bytes_received] = '\0';
            if (strlen(result) > 0) {
                printf("%s\n", result);
            }
        }
    }

    close(client_sock);
    return 0;
}