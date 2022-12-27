/**
 * TCP client
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/wait.h>
#include <errno.h>

#include "util.h"
#include "account_manager.h"

#define BUFF_SIZE 2048 /* size of buffer */

// server
#define SYS_LOGIN_SUCCESS "SYS_LOGIN_SUCCESS"
#define SYS_LOGIN_FAIL "SYS_LOGIN_FAIL"
#define SYS_ACC_NOT_FOUND "SYS_ACC_NOT_FOUND"
#define SYS_ACC_INACTIVE "SYS_ACC_INACTIVE"
#define SYS_MSG "SYS_MSG"
#define SYS_TERM "SYS_TERM"

// client
#define LOGIN_REQ "LOGIN_REQ"
#define LOGOUT_REQ "LOGOUT_REQ"
#define SEND_MSG_REQ "SEND_MSG_REQ"
#define VIEW_FULL_REQ "VIEW_FULL_REQ"

/**
 * Handle process signal
 * @param signo: signal number
*/
void sig_chld(int signo) {

    pid_t pid;
    int stat;

    // loop until no terminated child process
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {}
}

/**
 * Receive data from connected socket
 * @param fd: connected socket
 * @param buff: string buffer
 * @param size: size of buffer
 * @param flags: option flags
 * @return received bytes
*/
int recv_data(int fd, char *buff, int size, int flags) {
    int recv_bytes = recv(fd, buff, size, flags);
    if (recv_bytes < 0) {
        report_error();
    } else {
        buff[recv_bytes] = '\0';
    }
    return recv_bytes;
}

/**
 * Send data to connected socket
 * @param fd: connected socket
 * @param buff: string buffer
 * @param size: size of buffer
 * @param flags: option flags
 * @return sent bytes
*/
int send_data(int fd, char *buff, int size, int flags) {
    int send_bytes = send(fd, buff, size, flags);
    if (send_bytes < 0) {
        report_error();
    }
    return send_bytes;
}

int process_login(char *data) {
    if (strcmp(data, SYS_LOGIN_SUCCESS) == 0) {
        printf("Login successfully!\n");
        return 1;
    } else if (strcmp(data, SYS_LOGIN_FAIL) == 0) {
        printf("Incorrect password!\n");
        return 0;
    } else if (strcmp(data, SYS_ACC_NOT_FOUND) == 0) {
        printf("Username not found!\n");
        return 0;
    } else if (strcmp(data, SYS_ACC_INACTIVE) == 0) {
        printf("Inactive account!\n");
        return 0;
    }
}

int main(int argc, char *argv[]) {

    //* variables declaration
    char server_ip[20]; /* server ip address */
    int port_number; /* server opened port */

    int clientfd; /* client socket descriptor */
    struct sockaddr_in server_addr; /* server's address */
    int sin_size; /* size of address structure */

    char recv_buff[BUFF_SIZE], send_buff[BUFF_SIZE], data[BUFF_SIZE]; /* receive buffer, send buffer & data buffer */

    char username[MAX_LENGTH], password[MAX_LENGTH];
    int signed_in = 0; /* user is already signed in or not */
    int choice;

    pid_t pid;

    //* init
    if (argc == 5) {
        strcpy(server_ip, argv[1]);
        port_number = str_to_number(argv[2]);
        strcpy(username, argv[3]);
        strcpy(password, argv[4]);
    } else {
        printf("Usage: ./client <server_ip> <port_number> <username> <password>\n");
        exit(0);
    }

    //* fill server address info
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port_number);

    //* START
    printf("Client started!\n");

    //* create socket
    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        report_error();
        exit(0);
    }

    //* connect to server
    if (connect(clientfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        report_error(0);
        exit(0);
    }

    //* send login request
    memset(send_buff, '\0', sizeof(send_buff));
    strcpy(send_buff, LOGIN_REQ);
    send_buff[strlen(send_buff)] = '#';
    strcat(send_buff, username);
    send_buff[strlen(send_buff)] = '#';
    strcat(send_buff, password);

    send_data(clientfd, send_buff, strlen(send_buff), 0);

    //* receive login result
    recv_data(clientfd, recv_buff, BUFF_SIZE, 0);
    signed_in = process_login(recv_buff);

    //* fork sending process (child)
    if (signed_in) {
        //* establish a signal handler to catch SIGCHLD
        signal(SIGCHLD, sig_chld);

        //* create child process
        pid = fork();

        //* ------child process------
        if (pid == 0) {
            while (signed_in) {

                printf("=========Menu=========\n");
                printf("1. Send message\n");
                printf("2. View all messages\n");
                printf("Your choice:\n");
                scanf("%d", &choice);
                clear_stdin_buff();

                switch (choice) {
                    case 1:
                        printf("Message: ");
                        fgets(data, BUFF_SIZE, stdin);
                        data[strlen(data) - 1] = '\0';

                        memset(send_buff, '\0', sizeof(send_buff));
                        strcpy(send_buff, SEND_MSG_REQ);
                        send_buff[strlen(send_buff)] = '#';
                        strcat(send_buff, data);

                        send_data(clientfd, send_buff, strlen(send_buff), 0);

                        break;
                    case 2:
                        send_data(clientfd, VIEW_FULL_REQ, sizeof(VIEW_FULL_REQ), 0);
                        break;
                }
            }
        }
        //* ------end of child process------
    }

    //* receiving process (parent)
    while (signed_in) {
        recv_data(clientfd, data, BUFF_SIZE, 0);
        printf("Received:\n%s\n", data);
    }

    //* close socket
    close(clientfd);

    //* STOP
    printf("Client stopped!\n");

    return 0;
}