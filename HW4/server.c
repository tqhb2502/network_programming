/* UDP server */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client_server_conn.h"
#include "account_manager.h"
#include "util.h"

int main(int argc, char *argv[]) {

    // variable declaration
    int port_number; /* opened port */
    int server_sock; /* server socket file descriptor */

    char buff[BUFF_SIZE];

    struct sockaddr_in client_addr, client_addr_1, client_addr_2; /* client's addresses info*/
    int sin_size = sizeof(struct sockaddr_in);

    Account *acc_list = read_account_list(); // user account info list
    Account *client_1_acc, *client_2_acc; // signed in account

    int client_1_waiting_inchat = 0, client_2_waiting_inchat = 0; // already in chat status

    // init
    printf("Server is running...\n");

    if (argc == 2) {
        port_number = str_to_number(argv[1]);
    } else {
        printf("Usage: ./server port_number\n");
        exit(0);
    }

    // create local UDP socket
    server_sock = local_udp_server_sock(port_number);

    // get client's addresses info
    memset(&client_addr_1, '\0', sizeof(client_addr_1));
    memset(&client_addr_2, '\0', sizeof(client_addr_2));
    memset(&client_addr, '\0', sizeof(client_addr));

    h_recv_str(server_sock, &client_addr_1, buff);
    h_recv_str(server_sock, &client_addr_2, buff);

    // client 1 log in
    printf("Sign in for client 1\n");
    client_1_acc = acc_login_server_side(server_sock, &client_addr_1, acc_list, buff);

    // client 2 log in
    printf("Sign in for client 2\n");
    client_2_acc = acc_login_server_side(server_sock, &client_addr_2, acc_list, buff);

    // sign in done
    printf("Sign in done\n");

    // send communication ready message
    h_send_str(server_sock, &client_addr_1, OK_MSG);
    h_send_str(server_sock, &client_addr_2, OK_MSG);

    // communicate with client
    while (client_1_acc->is_signed_in || client_2_acc->is_signed_in) {

        // wait for datagram from client
        h_recv_str(server_sock, &client_addr, buff);

        if (ntohs(client_addr.sin_port) == ntohs(client_addr_1.sin_port)) {
            /* ---------- client 1 ---------- */

            if (strcmp(buff, CHANGE_PASSWD_CMD_MSG) == 0) {
                // change password
                change_passwd_server_side(server_sock, &client_addr_1, client_1_acc, acc_list, buff);
            } else if (strcmp(buff, SIGN_OUT_MSG) == 0) {
                // sign out
                sign_out_server_side(server_sock, &client_addr_1, client_1_acc, buff);
            } else if (strcmp(buff, CHAT_CMD_MSG) == 0) {
                // verify chatting requirement
                if (client_2_acc->is_signed_in) {
                    // client 2 is signed in
                    if (client_2_waiting_inchat) {
                        // client 2 is already in chat room, start chating
                        h_send_str(server_sock, &client_addr_1, OK_MSG);
                        h_send_str(server_sock, &client_addr_2, OK_MSG);
                        chat_room_server_side(server_sock, &client_addr_1, &client_addr_2, buff);
                        // chatting done, both clients are out of chat room
                        client_2_waiting_inchat = 0;
                    } else {
                        // client 2 is not waiting in chat room, send wait message
                        client_1_waiting_inchat = 1;
                        h_send_str(server_sock, &client_addr_1, WAIT_MSG);
                    }
                } else {
                    // client 2 is not signed in, reject request
                    h_send_str(server_sock, &client_addr_1, ERR_MSG);
                }
            }
        } else {
            /* ---------- client 2 ---------- */

            if (strcmp(buff, CHANGE_PASSWD_CMD_MSG) == 0) {
                // change password
                change_passwd_server_side(server_sock, &client_addr_2, client_2_acc, acc_list, buff);
            } else if (strcmp(buff, SIGN_OUT_MSG) == 0) {
                // sign out
                sign_out_server_side(server_sock, &client_addr_2, client_2_acc, buff);
            } else if (strcmp(buff, CHAT_CMD_MSG) == 0) {
                // verify chatting requirement
                if (client_1_acc->is_signed_in) {
                    // client 1 is signed in
                    if (client_1_waiting_inchat) {
                        // client 1 is already in chat room, start chating
                        h_send_str(server_sock, &client_addr_2, OK_MSG);
                        h_send_str(server_sock, &client_addr_1, OK_MSG);
                        chat_room_server_side(server_sock, &client_addr_1, &client_addr_2, buff);
                        // chatting done, both clients are out of chat room
                        client_1_waiting_inchat = 0;
                    } else {
                        // client 1 is not waiting in chat room, send wait message
                        client_2_waiting_inchat = 1;
                        h_send_str(server_sock, &client_addr_2, WAIT_MSG);
                    }
                } else {
                    // client 1 is not signed in, reject request
                    h_send_str(server_sock, &client_addr_2, ERR_MSG);
                }
            }
        }
    }

    printf("Server is stopped!\n");
    close(server_sock);
    return 0;
}