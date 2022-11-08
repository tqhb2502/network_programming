#ifndef __CLIENT_SERVER_CONN_H__
#define __CLIENT_SERVER_CONN_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "account_manager.h"

// size of buffer
#define BUFF_SIZE 1024

// server message
#define START_MSG "START"
#define OK_MSG "OK"
#define ERR_MSG "NOT_OK"
#define WAIT_MSG "WAIT"
#define ALREADY_USED_ACC_MSG "ALREADY_USED_ACC"
#define INACTIVE_ACC_MSG "INACTIVE_ACC"
#define BLOCK_ACC_MSG "BLOCK_ACC"

// client message
#define CHANGE_PASSWD_CMD_MSG "CHANGE_PASSWD_CMD"
#define CHAT_CMD_MSG "CHAT_CMD"
#define SIGN_OUT_MSG "SIGN_OUT"
#define SIGN_OUT_CONFIRM_MSG "bye"

// shared message
#define STOP_SERV_MSG "STOP_SERV"

int local_udp_server_sock(int port_number);
void h_recv_str(int receiver_sock, struct sockaddr_in *sender_addr, char *buff);
void h_send_str(int sender_sock, struct sockaddr_in *receiver_addr, char *buff);

Account* acc_login_server_side(int server_sock, struct sockaddr_in *client_addr,
    Account *acc_list, char *buff);
void acc_login_client_side(int client_sock, struct sockaddr_in *server_addr,
    int *is_logged_in, char *buff);

void change_passwd_server_side(int server_sock, struct sockaddr_in *client_addr,
    Account *user_account, Account *acc_list, char *buff);
void change_passwd_client_side(int client_sock, struct sockaddr_in *server_addr,
    char *buff);

void sign_out_server_side(int server_sock, struct sockaddr_in *client_addr,
    Account *user_account, char *buff);
void sign_out_client_side(int client_sock, struct sockaddr_in *server_addr,
    int *is_logged_in, char *buff);

void chat_room_server_side(int server_sock, struct sockaddr_in *client_addr_1,
    struct sockaddr_in *client_addr_2, char *buff);
void chat_room_client_side(int client_sock, struct sockaddr_in *server_addr,
    char *buff);
#endif
