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

/**
 * Deal with data receive from client
*/
int process_data(int fd, char *data, char *recv_buff, char *send_buff, Account *acc_list, int *client) {

    int data_len = strlen(data);

    int i = 0;
    while (data[i] != '#' && i < data_len) {
        i++;
    }

    // split command part and message part
    char cmd[BUFF_SIZE], msg[BUFF_SIZE];
    memset(cmd, '\0', sizeof(cmd));
    memset(msg, '\0', sizeof(msg));
    strncpy(cmd, data, i);
    if (i < data_len) {
        strcpy(msg, data + i + 1);
    }

    // execute command
    if (strcmp(cmd, LOGIN_REQ) == 0) {
        
        int j = 0;
        while (msg[j] != '#') {
            j++;
        }

        // split username part and password part
        char username[MAX_LENGTH], password[MAX_LENGTH];
        memset(username, '\0', sizeof(username));
        memset(password, '\0', sizeof(password));
        strncpy(username, msg, j);
        strcpy(password, msg + j + 1);

        // find account with username
        Account *acc = find_account(acc_list, username);

        // if account exists
        if (acc) {
            // account exists

            if (acc->status) {
                // active account

                // if password is correct
                if (strcmp(password, acc->password) == 0) {
                    // send OK
                    send_data(fd, SYS_LOGIN_SUCCESS, sizeof(SYS_LOGIN_SUCCESS), 0);
                } else {
                    // send FAIL
                    send_data(fd, SYS_LOGIN_FAIL, sizeof(SYS_LOGIN_FAIL), 0);
                    return 0;
                }
            } else {
                // blocked account
                send_data(fd, SYS_ACC_INACTIVE, sizeof(SYS_ACC_INACTIVE), 0);
                return 0;
            }
        } else {
            // account not found
            send_data(fd, SYS_ACC_NOT_FOUND, sizeof(SYS_ACC_NOT_FOUND), 0);
            return 0;
        }
    } else if (strcmp(cmd, SEND_MSG_REQ) == 0) {

        FILE *fp = fopen("groupchat.txt", "a");
        fputs(msg, fp);
        fputc('\n', fp);
        fclose(fp);

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (client[i] < 0) continue;
            if (client[i] == fd) continue;
            send_data(client[i], msg, sizeof(msg), 0);
        }
    } else if (strcmp(cmd, VIEW_FULL_REQ) == 0) {

        FILE *fp = fopen("groupchat.txt", "r");

        while (!feof(fp)) {
            memset(msg, '\0', sizeof(msg));
            fread(msg, BUFF_SIZE, 1, fp);
            send(fd, msg, strlen(msg), 0);
        }

        fclose(fp);
    }
    return 1;
}

int main(int argc, char *argv[]) {

    //* variables declaration
    int port_number = 5500; /* opened port */

    int listenfd; /* listen socket descriptor */
    int connfd; /* connected socket descriptor */
    int client[FD_SETSIZE]; /* store sockets connected to client */

    struct sockaddr_in server_addr; /* server address */
    struct sockaddr_in client_addr; /* client address */
    int sin_size; /* size of address structure */

    char recv_buff[BUFF_SIZE], send_buff[BUFF_SIZE], data[BUFF_SIZE]; /* receive buffer, send buffer & data buffer */

    fd_set checkfds, readfds, writefds, exceptfds; /* file descriptor set for select() to check */

    int ret_val; /* store result value of function */

    int maxfd; /* max value of file descriptor */

    Account *acc_list = read_account_list(); /* user account list */

    //* create listening socket
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

    //* initiate data structure
    for (int i = 0; i < FD_SETSIZE; i++) {
        client[i] = -1;
    }

    FD_ZERO(&checkfds);

    FD_SET(listenfd, &checkfds);
    maxfd = listenfd;

    //* START
    printf("Server started!\n");

    //* communicate with clients
    while (1) {

        // check if server has incomming connection
        readfds = checkfds;
        ret_val = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (ret_val < 0) {
            report_error();
            break;
        }

        // new connection
        if (FD_ISSET(listenfd, &readfds)) {

            // accept connection
            sin_size = sizeof(struct sockaddr_in);
            connfd = accept(listenfd, (struct sockaddr *) &client_addr, &sin_size);
            FD_SET(connfd, &checkfds);
            if (connfd > maxfd) {
                maxfd = connfd;
            }

            // add to client array
            int i;
            for (i = 0; i < FD_SETSIZE; i++) {
                if (client[i] < 0) {
                    client[i] = connfd;
                    break;
                }
            }

            if (i == FD_SETSIZE) {
                printf("Too many clients!\n");
            }
        }

        // check if server can read from clients
        for (int i = 0; i < FD_SETSIZE; i++) {

            if (client[i] < 0) continue;

            // can receive data
            if (FD_ISSET(client[i], &readfds)) {
                ret_val = recv_data(client[i], recv_buff, BUFF_SIZE, 0);
                if (ret_val > 0) {
                    strcpy(data, recv_buff);
                    ret_val = process_data(client[i], data, recv_buff, send_buff, acc_list, client);
                    if (ret_val == 0) {
                        FD_CLR(client[i], &checkfds);
                        close(client[i]);
                        client[i] = -1;
                    }
                }
            }
        }
    }

    //* close listen socket
    close(listenfd);

    //* STOP
    printf("Server stopped!\n");

    return 0;
}