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

#include "util.h"

#define BUFF_SIZE 2048
#define OK "SYS_MSG_OK"
#define ERR "SYS_MSG_ERR"
#define STOP "SYS_MSG_STOP"
#define EMPTY_STR "SYS_MSG_EMPTY_STR"
#define SEND_MSG_CMD "SYS_MSG_SEND_MSG_CMD"
#define SEND_FILE_CMD "SYS_MSG_SEND_FILE_CMD"
#define SENDING_FILE "SYS_MSG_SENDING_FILE"
#define DONE_SENDING_FILE "SYS_MSG_DONE_SENDING_FILE"
#define BLANK_LINE "SYS_MSG_BLANK_LINE"
#define STOP_PROG_CMD "SYS_MSG_STOP_PROG_CMD"

int main(int argc, char *argv[]) {

    //* variables declaration
    int port_number; /* opened port */

    int listenfd; /* listen socket descriptor */
    int connfd; /* connected socket descriptor */

    char buff[BUFF_SIZE];
    int recv_bytes, send_bytes;

    struct sockaddr_in serv_addr; /* server address */
    struct sockaddr_in client_addr; /* client address */
    int client_addr_len = sizeof(client_addr); /* size of client address */

    char num_str[BUFF_SIZE];
    char char_str[BUFF_SIZE];

    //* init
    if (argc == 2) {
        port_number = str_to_number(argv[1]);
    } else {
        printf("Usage: ./server <port_number>\n");
        exit(0);
    }

    //* create listening socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        report_error();
        exit(0);
    }

    //* fill server adddress
    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port_number);

    //* bind listening socket with server address
    if (bind(listenfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
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

        // accept connection request
        if ((connfd = accept(listenfd, (struct sockaddr *) &client_addr, &client_addr_len)) < 0) {
            report_error();
            exit(0);
        }

        // receive command
        recv_bytes = recv(connfd, buff, BUFF_SIZE, 0);
        buff[recv_bytes] = '\0';

        // message service
        if (strcmp(buff, SEND_MSG_CMD) == 0) {

            printf("------------------------\n");
            printf("Start message service!\n");

            while (1) {
                
                // receive message
                recv_bytes = recv(connfd, buff, BUFF_SIZE, 0);
                buff[recv_bytes] = '\0';

                // receive stop request --> stop service
                if (strcmp(buff, STOP) == 0) {
                    printf("Stop message service!\n");
                    break;
                }

                // process message
                if (char_number_split(buff, char_str, num_str)) {

                    // split successfully
                    send(connfd, OK, strlen(OK), 0);

                    // wait for sending allowance
                    recv_bytes = recv(connfd, buff, BUFF_SIZE, 0);
                    buff[recv_bytes] = '\0';

                    if (strcmp(buff, OK) == 0) {

                        // send first result
                        if (strlen(num_str) > 0) {
                            send(connfd, num_str, strlen(num_str), 0);
                        } else {
                            send(connfd, EMPTY_STR, strlen(EMPTY_STR), 0);
                        }

                        // wait for sending allowance
                        recv_bytes = recv(connfd, buff, BUFF_SIZE, 0);
                        buff[recv_bytes] = '\0';

                        if (strcmp(buff, OK) == 0) {
                            // send second result
                            if (strlen(char_str) > 0) {
                                send(connfd, char_str, strlen(char_str), 0);
                            } else {
                                send(connfd, EMPTY_STR, strlen(EMPTY_STR), 0);
                            }
                        }
                    }
                } else {
                    // split fail
                    send(connfd, ERR, strlen(ERR), 0);
                }
            }
        }

        // file service
        if (strcmp(buff, SEND_FILE_CMD) == 0) {

            printf("------------------------\n");
            printf("Start file service!\n");
            printf("File content:\n");
            
            while (1) {

                // receive file status
                recv_bytes = recv(connfd, buff, BUFF_SIZE, 0);
                buff[recv_bytes] = '\0';

                if (strcmp(buff, SENDING_FILE) == 0) {
                    // file is sending

                    // send sending allowance
                    send(connfd, OK, strlen(OK), 0);

                    // receive data
                    recv_bytes = recv(connfd, buff, BUFF_SIZE, 0);
                    buff[recv_bytes] = '\0';

                    // display data
                    if (strcmp(buff, BLANK_LINE) != 0) {
                        fputs(buff, stdout);
                        fflush(stdout);
                    }

                    // ok, next loop
                    send(connfd, OK, strlen(OK), 0);
                } else {
                    // sending file is done
                    break;
                }
            }

            printf("\nStop file service!\n");
        }

        // stop program --> close socket & stop
        if (strcmp(buff, STOP_PROG_CMD) == 0) {
            close(connfd);
            break;
        }

        // close socket
        close(connfd);
    }

    //* close socket
    close(listenfd);

    printf("Server stopped!\n");
    return 0;
}