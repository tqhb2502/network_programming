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

#include "util.h"
#include "account_manager.h"

#define BUFF_SIZE 2048
#define SYS_OK "#SYS_OK"
#define SYS_FAIL "#SYS_FAIL"
#define SYS_BYE "#SYS_BYE"
#define SYS_BLOCK "#SYS_BLOCK"
#define SYS_IS_BLOCKED "#SYS_IS_BLOCKED"
#define LOG_OUT_MSG "bye"

int main(int argc, char *argv[]) {

    //* variables declaration
    char server_ip[20]; /* server ip address */
    int port_number; /* server opened port */

    int clientfd; /* client socket descriptor */
    struct sockaddr_in server_addr; /* server's address */

    char buff[BUFF_SIZE];
    int recv_bytes, send_bytes; /* number of bytes send and receive */

    int signed_in = 0; /* user is already signed in or not */

    Account *acc_list = read_account_list();

    //* init
    if (argc == 3) {
        strcpy(server_ip, argv[1]);
        port_number = str_to_number(argv[2]);
    } else {
        printf("Usage: ./client <server_ip> <port_number>\n");
        exit(0);
    }

    //* fill server address info
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port_number);

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

    //* communicate with server
    while (1) {
        if (signed_in) {

            printf("Type '%s' to log out!\n", LOG_OUT_MSG);

            fgets(buff, BUFF_SIZE, stdin);
            buff[strlen(buff) - 1] = '\0';

            if (strcmp(buff, LOG_OUT_MSG) == 0) {
                signed_in = 0;
            }
        } else {

            printf("---Leave username empty to quit---\n");

            // input username
            printf("Username: ");
            fgets(buff, BUFF_SIZE, stdin);
            buff[strlen(buff) - 1] = '\0';

            if (strlen(buff) == 0) {
                // username is left empty -> quit
                send(clientfd, SYS_BYE, sizeof(SYS_BYE), 0);
                break;
            } else {
                // send username to server
                send(clientfd, buff, sizeof(buff), 0);
            }

            // find account for fail sign-in attemp count
            Account *acc = find_account(acc_list, buff);

            // receive status
            recv_bytes = recv(clientfd, buff, BUFF_SIZE, 0);
            buff[recv_bytes] = '\0';

            if (strcmp(buff, SYS_OK) == 0) {
                // username is found

                // input password
                printf("Password: ");
                fgets(buff, BUFF_SIZE, stdin);
                buff[strlen(buff) - 1] = '\0';

                // send to server
                send(clientfd, buff, sizeof(buff), 0);

                // receive status
                recv_bytes = recv(clientfd, buff, BUFF_SIZE, 0);
                buff[recv_bytes] = '\0';

                if (strcmp(buff, SYS_OK) == 0) {
                    // correct password
                    printf("Login successfully!\n");
                    signed_in = 1;
                    acc->consecutive_failed_sign_in = 0;
                    send(clientfd, SYS_OK, sizeof(SYS_OK), 0);
                } else {
                    // incorrect password

                    acc->consecutive_failed_sign_in++;
                    
                    if (acc->consecutive_failed_sign_in <= 3) {
                        // <= 3 consecutive fails
                        printf("Incorrect password!\n");
                        send(clientfd, SYS_OK, sizeof(SYS_OK), 0);
                    } else {
                        // more than 3 consecutive fails
                        printf("Too many fail attemps. Block this account!\n");
                        send(clientfd, SYS_BLOCK, sizeof(SYS_BLOCK), 0);
                    }
                }
            } else if (strcmp(buff, SYS_IS_BLOCKED) == 0) {
                // found account is blocked
                printf("Account is blocked!\n");
            } else {
                // username not found
                printf("Username not found!\n");
            }
        }
    }

    //* close socket
    close(clientfd);

    printf("Client stopped!\n");
    return 0;
}