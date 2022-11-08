/**
 * Client-server communication utilities
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "account_manager.h"
#include "client_server_conn.h"

/**
 * Create a udp socket and bind it with <localhost>:<port_number>
*/
int local_udp_server_sock(int port_number) {

    int server_sock;
    struct sockaddr_in server_addr;

    // create socket
    if ((server_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        report_error();
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

        report_error();
        exit(0);
    }

    return server_sock;
}

/**
 * receive datagram
*/
void h_recv_str(int receiver_sock, struct sockaddr_in *sender_addr, char *buff) {
    int sin_size = sizeof(struct sockaddr_in);
    int bytes_received = recvfrom(receiver_sock, buff, BUFF_SIZE, 0,
        (struct sockaddr *) sender_addr, &sin_size);
    buff[bytes_received] = '\0';
}

/**
 * send datagram
*/
void h_send_str(int sender_sock, struct sockaddr_in *receiver_addr, char *buff) {
    sendto(sender_sock, buff, strlen(buff), 0,
        (struct sockaddr *) receiver_addr, sizeof(struct sockaddr));
}

/**
 * account login service in server side
*/
Account* acc_login_server_side(int server_sock, struct sockaddr_in *client_addr,
    Account *acc_list, char *buff) {

    Account *user_account;

    while (1) {

        // start log-in service
        h_send_str(server_sock, client_addr, OK_MSG);

        // receive username
        h_recv_str(server_sock, client_addr, buff);
        user_account = find_account(acc_list, buff);

        if (user_account) {
            // username exists

            // send OK message
            h_send_str(server_sock, client_addr, OK_MSG);

            // receive password
            h_recv_str(server_sock, client_addr, buff);

            if (strcmp(user_account->password, buff) == 0) {
                // correct password
                user_account->consecutive_failed_sign_in = 0;

                if (user_account->is_signed_in) {
                    // account is already used by the another client
                    h_send_str(server_sock, client_addr, ALREADY_USED_ACC_MSG);
                } else {
                    // account is not in use currently
                    if (is_active_account(acc_list, user_account->username)) {
                        // active account
                        user_account->is_signed_in = 1;
                        h_send_str(server_sock, client_addr, OK_MSG);
                        break;
                    } else {
                        // inactive account
                        h_send_str(server_sock, client_addr, INACTIVE_ACC_MSG);
                    }
                }
            } else {
                // incorrect password
                user_account->consecutive_failed_sign_in++;
                if (user_account->consecutive_failed_sign_in < 3) {
                    // less than 3 times
                    h_send_str(server_sock, client_addr, ERR_MSG);
                } else {
                    // 3 times
                    user_account->status = 0;
                    write_to_file(acc_list);
                    h_send_str(server_sock, client_addr, BLOCK_ACC_MSG);
                }
            }
        } else {
            // username doesn't exist
            h_send_str(server_sock, client_addr, ERR_MSG);
        }
    }

    return user_account;
}

/**
 * account login service in client side
*/
void acc_login_client_side(int client_sock, struct sockaddr_in *server_addr,
    int *is_logged_in, char *buff) {

    while (1) {

        // start login
        h_recv_str(client_sock, server_addr, buff);

        if (strcmp(buff, OK_MSG) == 0) {

            // enter username
            printf("Username: ");
            fgets(buff, BUFF_SIZE, stdin);
            buff[strlen(buff) - 1] = '\0';
            h_send_str(client_sock, server_addr, buff);

            // receive server's message
            h_recv_str(client_sock, server_addr, buff);

            if (strcmp(buff, OK_MSG) == 0) {
                // username has been found

                // enter password
                printf("Insert password: ");
                fgets(buff, BUFF_SIZE, stdin);
                buff[strlen(buff) - 1] = '\0';
                h_send_str(client_sock, server_addr, buff);

                // receive server's message
                h_recv_str(client_sock, server_addr, buff);

                if (strcmp(buff, OK_MSG) == 0) {
                    // correct password, active account
                    *is_logged_in = 1;
                    printf("OK\n");
                    break;
                } else if (strcmp(buff, ALREADY_USED_ACC_MSG) == 0) {
                    // already used account
                    printf("Account is already used\n");
                } else if (strcmp(buff, INACTIVE_ACC_MSG) == 0) {
                    // inactive account
                    printf("Account not ready\n");
                } else if (strcmp(buff, BLOCK_ACC_MSG) == 0) {
                    // got password wrong 3 times, block account
                    printf("Account is blocked\n");
                } else {
                    // incorrect password
                    printf("Not OK\n");
                }
            } else {
                // username not found
                printf("Username not found! Please try again!\n");
            }
        }
    }
}

/**
 * change password service server side
*/
void change_passwd_server_side(int server_sock, struct sockaddr_in *client_addr,
    Account *user_account, Account *acc_list, char *buff) {

    char char_str[BUFF_SIZE];
    char num_str[BUFF_SIZE];
    
    // receive new password
    h_recv_str(server_sock, client_addr, buff);

    // process password
    if (char_number_split(buff, char_str, num_str)) {
        // if ok, apply new password
        strcpy(user_account->password, buff);
        write_to_file(acc_list);
        // send password back to client
        h_send_str(server_sock, client_addr, OK_MSG);
        h_send_str(server_sock, client_addr, num_str);
        h_send_str(server_sock, client_addr, char_str);
    } else {
        // error occured, send error message back to client
        h_send_str(server_sock, client_addr, ERR_MSG);
    }
}

/**
 * change password service client side
*/
void change_passwd_client_side(int client_sock, struct sockaddr_in *server_addr,
    char *buff) {

    // enter new password
    printf("New password: ");
    fgets(buff, BUFF_SIZE, stdin);
    buff[strlen(buff) - 1] = '\0';

    // send new password to server
    h_send_str(client_sock, server_addr, buff);

    // receive message from server
    h_recv_str(client_sock, server_addr, buff);

    if (strcmp(buff, OK_MSG) == 0) {
        // receive ok message

        h_recv_str(client_sock, server_addr, buff);
        if (strlen(buff) > 0) {
            printf("%s\n", buff);
        }

        h_recv_str(client_sock, server_addr, buff);
        if (strlen(buff) > 0) {
            printf("%s\n", buff);
        }
    } else {
        // receive error message

        printf("Error\n");
    }
}

/**
 * sign out service server side
*/
void sign_out_server_side(int server_sock, struct sockaddr_in *client_addr,
    Account *user_account, char *buff) {

    // send ok message, expecting sign out confirm message
    h_send_str(server_sock, client_addr, OK_MSG);

    // receive message from client
    h_recv_str(server_sock, client_addr, buff);
    if (strcmp(buff, SIGN_OUT_CONFIRM_MSG) == 0) {
        // receive sign out confirm message, accept and send username
        user_account->is_signed_in = 0;
        h_send_str(server_sock, client_addr, OK_MSG);
        h_send_str(server_sock, client_addr, user_account->username);
    } else {
        // not receive sign out confirm message, refuse
        h_send_str(server_sock, client_addr, ERR_MSG);
    }
}

/**
 * sign out service client side
*/
void sign_out_client_side(int client_sock, struct sockaddr_in *server_addr,
    int *is_logged_in, char *buff) {

    // receive message from server
    h_recv_str(client_sock, server_addr, buff);
    if (strcmp(buff, OK_MSG) == 0) {
        // if ok
        printf("Enter 'bye' to sign out\n");
        fgets(buff, BUFF_SIZE, stdin);
        buff[strlen(buff) - 1] = '\0';
        h_send_str(client_sock, server_addr, buff);

        // receive message from server
        h_recv_str(client_sock, server_addr, buff);
        if (strcmp(buff, OK_MSG) == 0) {
            // if ok, receive username, print notification, sign out
            h_recv_str(client_sock, server_addr, buff);
            printf("Goodbye %s\n", buff);
            *is_logged_in = 0;
        } else {
            printf("You are not signed out\n");
        }
    }
}

/**
 * chat room service server side
*/
void chat_room_server_side(int server_sock, struct sockaddr_in *client_addr_1,
    struct sockaddr_in *client_addr_2, char *buff) {

    struct sockaddr_in client_addr;
    int received_stop_request = 0;
    int sent_last_message = 0;

    while (!received_stop_request || !sent_last_message) {

        // receive message from client
        h_recv_str(server_sock, &client_addr, buff);

        if (ntohs(client_addr.sin_port) == ntohs(client_addr_1->sin_port)) {
            /* ----- Client 1 ----- */

            if (strcmp(buff, STOP_SERV_MSG) == 0) {
                // receive stop service message
                // print notification, send stop service message to client 2
                if (!received_stop_request) {
                    printf("Received stop service message\n");
                    h_send_str(server_sock, client_addr_2, STOP_SERV_MSG);
                    received_stop_request = 1;
                } else {
                    h_send_str(server_sock, client_addr_2, STOP_SERV_MSG);
                    sent_last_message = 1;
                }
            } else {
                // receive normal message, send message to client 2
                if (!received_stop_request) {
                    h_send_str(server_sock, client_addr_2, buff);
                } else {
                    h_send_str(server_sock, client_addr_2, STOP_SERV_MSG);
                    sent_last_message = 1;
                }
            }
        } else {
            /* ----- Client 2 ----- */

            if (strcmp(buff, STOP_SERV_MSG) == 0) {
                // receive stop service message
                // print notification, send stop service message to client 1
                if (!received_stop_request) {
                    printf("Received stop service message\n");
                    h_send_str(server_sock, client_addr_1, STOP_SERV_MSG);
                    received_stop_request = 1;
                } else {
                    h_send_str(server_sock, client_addr_1, STOP_SERV_MSG);
                    sent_last_message = 1;
                }
            } else {
                // receive normal message, send message to client 1
                if (!received_stop_request) {
                    h_send_str(server_sock, client_addr_1, buff);
                } else {
                    h_send_str(server_sock, client_addr_1, STOP_SERV_MSG);
                    sent_last_message = 1;
                }
            }
        }
    }

    printf("Stop chatting service!\n");
}

/**
 * chat room service client side
*/
void chat_room_client_side(int client_sock, struct sockaddr_in *server_addr,
    char *buff) {

    int received_stop_request = 0;

    while (!received_stop_request) {

        // user enter message
        printf("Your message: ");
        fgets(buff, BUFF_SIZE, stdin);
        buff[strlen(buff) - 1] = '\0';

        // process message
        if (strlen(buff) == 0) {
            // if message is empty string, send stop message and stop process
            h_send_str(client_sock, server_addr, STOP_SERV_MSG);
            printf("Stop service message sent!\n");
        } else {
            // non-empty message -> send to server
            h_send_str(client_sock, server_addr, buff);
            printf("Message sent!\n");
        }

        // receive message forwarded by server from the another client
        h_recv_str(client_sock, server_addr, buff);

        // process message
        if (strcmp(buff, STOP_SERV_MSG) == 0) {
            // stop service message
            printf("Stop service message received!\n");
            printf("Stop chatting!\n");
            received_stop_request = 1;
        } else {
            // normal message
            printf("Friend message: %s\n", buff);
        }
    }
}
