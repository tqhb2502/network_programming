/* UDP client */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "client_server_conn.h"

int main(int argc, char *argv[]) {

    int client_sock; /* client socket file descriptor */
    char buff[BUFF_SIZE];
    struct sockaddr_in server_addr; /* server's address info */
    char server_ip[20]; /* server's IP */
    int port_number; /* server's opened port */
    int is_logged_in = 0; /* 0: not log in yet, 1: has logged in*/
    int comm_ready = 0; /* communication ready */
    int choice; /* user's choice */
    char ch;

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
        report_error();
        exit(0);
    }

    // fill server info
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // send start message to server
    h_send_str(client_sock, &server_addr, START_MSG);

    // log in
    acc_login_client_side(client_sock, &server_addr, &is_logged_in, buff);

    // wait for cummunication ready message from server
    printf("Waiting for server and the another client to ready...\n");
    h_recv_str(client_sock, &server_addr, buff);
    if (strcmp(buff, OK_MSG) == 0) {
        comm_ready = 1;
    }

    // comunicate with server
    while (is_logged_in && comm_ready) {

        // server service
        printf("-------Service-------\n");
        printf("1. Change password\n");
        printf("2. Chat\n");
        printf("3. Sign out\n");

        // user choice
        printf("Your choice: ");
        scanf("%d", &choice);
        while ((ch = getchar()) != EOF && ch != '\n');

        switch (choice) {

        // change password
        case 1:
            // send request
            h_send_str(client_sock, &server_addr, CHANGE_PASSWD_CMD_MSG);
            change_passwd_client_side(client_sock, &server_addr, buff);
            break;
        
        // chat
        case 2:
            // send request
            h_send_str(client_sock, &server_addr, CHAT_CMD_MSG);
            // receive message from server
            h_recv_str(client_sock, &server_addr, buff);
            if (strcmp(buff, ERR_MSG) == 0) {
                // the another client is not signed in
                printf("The another client is currently not signed in. Cannot start chatting!\n");
            } else if (strcmp(buff, WAIT_MSG) == 0) {
                // the another client is not waiting in chat room
                printf("Waiting for the another client to enter chat room...\n");
                // waiting for start chatting message
                h_recv_str(client_sock, &server_addr, buff);
                if (strcmp(buff, OK_MSG) == 0) {
                    chat_room_client_side(client_sock, &server_addr, buff);
                }
            } else {
                // the another client is signed in and already in chat room, start chatting
                chat_room_client_side(client_sock, &server_addr, buff);
            }
            break;
        
        // sign out
        case 3:
            // send request
            h_send_str(client_sock, &server_addr, SIGN_OUT_MSG);
            sign_out_client_side(client_sock, &server_addr, &is_logged_in, buff);
            break;
        }
    }

    printf("Client is stopped!\n");
    close(client_sock);
    return 0;
}