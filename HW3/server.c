/* UDP server */
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

    int port_number; /* opened port */
    int server_sock; /* server socket file descriptor */

    char buff[BUFF_SIZE], ok_msg[10], err_msg[10];
    int bytes_sent, bytes_received;

    struct sockaddr_in server_addr; /* server's address info */
    struct sockaddr_in client_addr, client_addr_1, client_addr_2; /* client's addresses info*/
    int sin_size = sizeof(struct sockaddr_in);

    char char_str[BUFF_SIZE];
    char num_str[BUFF_SIZE];

    // init
    printf("Server is running...\n");

    strcpy(ok_msg, "ok");
    ok_msg[strlen(ok_msg)] = '\0';
    strcpy(err_msg, "error");
    err_msg[strlen(err_msg)] = '\0';

    if (argc == 2) {
        port_number = str_to_number(argv[1]);
    } else {
        printf("Usage: ./server port_number\n");
        exit(0);
    }
    
    // create socket
    if ((server_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error");
        printf("\n");
        exit(0);
    }

    // fill server info
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // bind socket with server address
    if (bind(server_sock, (struct sockaddr *) &server_addr,
        sizeof(struct sockaddr)) < 0) {

        perror("Error");
        printf("\n");
        exit(0);
    }

    // get client's addressed info
    memset(&client_addr_1, '\0', sizeof(client_addr_1));
    memset(&client_addr_2, '\0', sizeof(client_addr_2));
    memset(&client_addr, '\0', sizeof(client_addr));

    bytes_received = recvfrom(server_sock, buff, BUFF_SIZE, 0,
        (struct sockaddr *) &client_addr_1, &sin_size);

    bytes_received = recvfrom(server_sock, buff, BUFF_SIZE, 0,
        (struct sockaddr *) &client_addr_2, &sin_size);

    while (1) {

        // wait for datagram package from client
        bytes_received = recvfrom(server_sock, buff, BUFF_SIZE, 0,
            (struct sockaddr *) &client_addr, &sin_size);
        buff[bytes_received] = '\0';

        if (ntohs(client_addr.sin_port) == ntohs(client_addr_1.sin_port)) {
            /* ---------- client 1 ---------- */

            /**
             * if receive stop message, send stop message to the another client
             * and then stop process
            */
            if (strcmp(buff, "stop") == 0) {
                bytes_sent = sendto(server_sock, buff, strlen(buff), 0,
                    (struct sockaddr *) &client_addr_2, sizeof(struct sockaddr));
                printf("Server is stopped\n");
                break;
            }

            // process data
            if (char_number_split(buff, char_str, num_str)) {
                // if ok, send data to client 2
                bytes_sent = sendto(server_sock, ok_msg, strlen(ok_msg), 0,
                    (struct sockaddr *) &client_addr_2, sizeof(struct sockaddr));
                bytes_sent = sendto(server_sock, num_str, strlen(num_str), 0,
                    (struct sockaddr *) &client_addr_2, sizeof(struct sockaddr));
                bytes_sent = sendto(server_sock, char_str, strlen(char_str), 0,
                    (struct sockaddr *) &client_addr_2, sizeof(struct sockaddr));
            } else {
                // error occured, send error message back to client 2
                bytes_sent = sendto(server_sock, err_msg, strlen(err_msg), 0,
                    (struct sockaddr *) &client_addr_2, sizeof(struct sockaddr));
            }
        } else {
            /* ---------- client 2 ---------- */

            /**
             * if receive stop message, send stop message to the another client
             * and then stop process
            */
            if (strcmp(buff, "stop") == 0) {
                bytes_sent = sendto(server_sock, buff, strlen(buff), 0,
                    (struct sockaddr *) &client_addr_1, sizeof(struct sockaddr));
                printf("Server is stopped\n");
                break;
            }

            // process data
            if (char_number_split(buff, char_str, num_str)) {
                // if ok, send data to client 1
                bytes_sent = sendto(server_sock, ok_msg, strlen(ok_msg), 0,
                    (struct sockaddr *) &client_addr_1, sizeof(struct sockaddr));
                bytes_sent = sendto(server_sock, num_str, strlen(num_str), 0,
                    (struct sockaddr *) &client_addr_1, sizeof(struct sockaddr));
                bytes_sent = sendto(server_sock, char_str, strlen(char_str), 0,
                    (struct sockaddr *) &client_addr_1, sizeof(struct sockaddr));
            } else {
                // error occured, send error message back to client 1
                bytes_sent = sendto(server_sock, err_msg, strlen(err_msg), 0,
                    (struct sockaddr *) &client_addr_1, sizeof(struct sockaddr));
            }
        }
    }

    close(server_sock);
    return 0;
}