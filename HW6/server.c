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

#define BUFF_SIZE 2048
#define SYS_OK "#SYS_OK"
#define SYS_FAIL "#SYS_FAIL"
#define SYS_BYE "#SYS_BYE"
#define SYS_BLOCK "#SYS_BLOCK"
#define SYS_IS_BLOCKED "#SYS_IS_BLOCKED"

/**
 * Handle process signal
 * @param signo: signal number
*/
void sig_chld(int signo);

int main(int argc, char *argv[]) {

    //* variables declaration
    int port_number; /* opened port */

    int listenfd; /* listen socket descriptor */
    int connfd; /* connected socket descriptor */

    char buff[BUFF_SIZE];
    int recv_bytes, send_bytes; /* number of bytes send and receive */

    struct sockaddr_in server_addr; /* server address */
    struct sockaddr_in client_addr; /* client address */
    int sin_size; /* size of client address and server address */

    pid_t pid; /* child process id */

    Account *acc_list = read_account_list(); /* user account list */

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

    //* establish a signal handler to catch SIGCHLD
    signal(SIGCHLD, sig_chld);

    printf("Server started!\n");

    //* communicate with clients
    while (1) {

        sin_size = sizeof(struct sockaddr_in);

        // accept connection request
        if ((connfd = accept(listenfd, (struct sockaddr *) &client_addr, &sin_size)) < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                report_error();
                exit(0);
            }
        }

        // create new child process
        printf("You got a connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        pid = fork();

        //* -----------child process-----------
        // in child process, fork() is also called and returns 0 if success
        while (pid == 0) {

            // close shared listen socket
            close(listenfd);

            // receive client's username
            recv_bytes = recv(connfd, buff, BUFF_SIZE, 0);
            buff[recv_bytes] = '\0';
        
            // end child process if receive empty username
            if (strcmp(buff, SYS_BYE) == 0) {
                exit(0);
            }

            // find account with username
            Account *acc = find_account(acc_list, buff);

            // if account exists
            if (acc) {
                // account exists

                if (acc->status) {
                    // active account

                    // send OK status to client
                    send(connfd, SYS_OK, sizeof(SYS_OK), 0);

                    // receive password
                    recv_bytes = recv(connfd, buff, BUFF_SIZE, 0);
                    buff[recv_bytes] = '\0';

                    // if password is correct
                    if (strcmp(buff, acc->password) == 0) {
                        // send OK
                        send(connfd, SYS_OK, sizeof(SYS_OK), 0);
                    } else {
                        // send FAIL
                        send(connfd, SYS_FAIL, sizeof(SYS_FAIL), 0);
                    }

                    // receive status from client
                    recv_bytes = recv(connfd, buff, BUFF_SIZE, 0);
                    buff[recv_bytes] = '\0';

                    // receive block request -> block current account
                    if (strcmp(buff, SYS_BLOCK) == 0) {
                        acc->status = 0;
                        write_to_file(acc_list);
                    }
                } else {
                    // blocked account
                    send(connfd, SYS_IS_BLOCKED, sizeof(SYS_IS_BLOCKED), 0);
                }
            } else {
                // account not found
                send(connfd, SYS_FAIL, sizeof(SYS_FAIL), 0);
            }
        }
        //* -----------end of child process-----------

        // close connected socket that is already handled
        close(connfd);
    }

    //* close listen socket
    close(listenfd);

    printf("Server stopped!\n");
    return 0;
}

void sig_chld(int signo) {

    pid_t pid;
    int stat;

    // loop until no terminated child process
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        printf("Child process %d terminated\n", pid);
    }
}