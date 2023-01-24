/**
 * Ở bài này em làm theo mô hình lai giữa client-server và peer-to-peer
 * nên trước khi dùng câu lệnh "./mcast <ip_address> <port_number>" để khởi động client,
 * cần dùng câu lệnh "./server" để khởi động server trước ạ!
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <pthread.h>

#include "util.h"

#define BUFF_SIZE 30
#define SERVER_PORT_NUMBER 9999

#define SYS_HELLO "#SYS_HELLO#"
#define SYS_ADDR "#SYS_ADDR#"
#define SYS_DONE "#SYS_DONE#"
#define SYS_QUIT "#SYS_QUIT#"

#define CMD_MTAB "print mtable"
#define CMD_QUIT "quit"

typedef struct client_info_t {
    int conn_socket; // socket connected to client
    struct sockaddr_in listen_address; // the address of listening socket of client
    int is_active; // indicate that client is active or not
} client_info;

typedef struct data_package_t {
    char control_signal[BUFF_SIZE];
    struct sockaddr_in address;
} data_package;

int listen_fd; /* listening socket of this client */
int serv_conn_fd; /* socket connected to server */

client_info client[FD_SETSIZE];

pthread_mutex_t mutex; /* uses to lock shared memory */

/**
 * every 30s, check if there are "hello" messages from other clients.
 * if the "hello" message from any client is not received,
 * this client will be considered inactive.
*/
void *active_client_handler();

/**
 * create listening socket
 * listen for connection from other clients
*/
void *listening_socket_handler(void *param);

/**
 * every 10s, send "hello" message to other clients and server
 * indicate that this client is still active
*/
void *active_signal_sender();

/**
 * copy struct sockaddr_in from src to dest
*/
void copy_sockaddr_in(struct sockaddr_in *dest, struct sockaddr_in *src);

int main(int argc, char *argv[]) {

    //* variables declaration
    int conn_fd; /* connected socket */

    struct sockaddr_in server_addr; /* server address */
    struct sockaddr_in client_addr; /* connected client address */
    int sin_size; /* stores address size */

    char ip_address[20];
    int port_number;

    data_package pkg;
    data_package my_addr_pkg; /* stores listening socket address of this client */
    int dpkg_size = sizeof(data_package);

    char cmd[1024]; /* stores user's command */

    //* init
    if (argc == 3) {
        strcpy(ip_address, argv[1]);
        port_number = str_to_number(argv[2]);
    } else {
        printf("Usage: ./mcast <ip_address> <port_number>\n");
        exit(0);
    }

    //* fill my address
    memset(&my_addr_pkg, '\0', sizeof(my_addr_pkg));
    my_addr_pkg.address.sin_family = AF_INET;
    my_addr_pkg.address.sin_addr.s_addr = inet_addr(ip_address);
    my_addr_pkg.address.sin_port = htons(port_number);

    //* init client info array
    for (int i = 0; i < FD_SETSIZE; i++) {
        client[i].conn_socket = -1;
    }

    //* init mutex
    pthread_mutex_init(&mutex, NULL);

    //* -------- GET CLIENT LIST --------
    // create socket
    if ((serv_conn_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        report_error();
        exit(0);
    }

    // fill server adddress
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT_NUMBER);

    // connect to server
    if (connect(serv_conn_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        printf("Cần sử dụng câu lệnh \"./server\" để khởi động server trước!\n");
        exit(0);
    }

    // send init package 
    send(serv_conn_fd, &my_addr_pkg, sizeof(my_addr_pkg), 0);

    // receive client list
    while (1) {

        recv(serv_conn_fd, &pkg, dpkg_size, 0);

        if (strcmp(pkg.control_signal, SYS_ADDR) == 0) {

            // connect to the client
            if ((conn_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                report_error();
                continue;
            }
            
            if (connect(conn_fd, (struct sockaddr *) &pkg.address, sizeof(pkg.address)) < 0) {
                report_error();
                continue;
            }

            // send init package
            send(conn_fd, &my_addr_pkg, sizeof(my_addr_pkg), 0);

            // store info
            for (int i = 0; i < FD_SETSIZE; i++) {
                
                if (client[i].conn_socket == -1) {

                    client[i].conn_socket = conn_fd;
                    copy_sockaddr_in(&client[i].listen_address, &pkg.address);

                    break;
                }
            }
        } else {
            printf("Receiving client list done!\n");
            printf("------------------\n");
            break;
        }
    }

    //* -------- FUNCTION THREADS --------
    // listening socket handler
    pthread_t listening_socket_thread;
    if (pthread_create(&listening_socket_thread, NULL, listening_socket_handler, (void *) &my_addr_pkg) < 0) {
        report_error();
        exit(0);
    }
    pthread_detach(listening_socket_thread);

    // active client handler
    pthread_t active_client_thread;
    if (pthread_create(&active_client_thread, NULL, active_client_handler, NULL) < 0) {
        report_error();
        exit(0);
    }
    pthread_detach(active_client_thread);

    // active signal sender
    pthread_t active_signal_thread;
    if (pthread_create(&active_signal_thread, NULL, active_signal_sender, NULL) < 0) {
        report_error();
        exit(0);
    }
    pthread_detach(active_signal_thread);

    //* -------- HANDLE USER'S COMMAND --------
    while (1) {

        // input command
        printf("Command: ");
        fgets(cmd, 1023, stdin);
        cmd[strlen(cmd) - 1] = '\0';

        // handle
        if (strcmp(cmd, CMD_MTAB) == 0) {
            
            printf("List of neighbours:\n");

            for (int i = 0; i < FD_SETSIZE; i++) {

                if (client[i].conn_socket != -1) {

                    printf("%s:%d\n",
                        inet_ntoa(client[i].listen_address.sin_addr), ntohs(client[i].listen_address.sin_port));
                }
            }

            printf("------------------\n");
        } else if (strcmp(cmd, CMD_QUIT) == 0) {
            
            // terminate threads
            pthread_cancel(listening_socket_thread);
            pthread_cancel(active_client_thread);
            pthread_cancel(active_signal_thread);

            // send quit signal and close sockets
            memset(&pkg, '\0', sizeof(pkg));
            strcpy(pkg.control_signal, SYS_QUIT);
            // server
            send(serv_conn_fd, &pkg, sizeof(pkg), 0);
            close(serv_conn_fd);
            // other clients
            for (int i = 0; i < FD_SETSIZE; i++) {

                if (client[i].conn_socket != -1) {
                    
                    send(client[i].conn_socket, &pkg, sizeof(pkg), 0);
                    close(client[i].conn_socket);
                }
            }

            // close listening socket
            close(listen_fd);

            // destroy mutex
            pthread_mutex_destroy(&mutex);

            printf("Bye\n");
            printf("------------------\n");
            break;
        } else {
            printf("Wrong command\n");
            printf("------------------\n");
        }
    }

    return 0;
}

void *active_client_handler() {

    fd_set readfds;
    int maxfd;
    struct timeval tv;

    data_package pkg;
    int dpkg_size = sizeof(data_package);

    int readable;

    //* init timeval
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    //* check for client's message every 30s
    while (1) {

        // sleeps this thread for 30s
        sleep(30);

        // consider all clients as inactive
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < FD_SETSIZE; i++) {

            if (client[i].conn_socket != -1) {

                client[i].is_active = 0;
            }
        }
        pthread_mutex_unlock(&mutex);

        // set readable to start the loop
        readable = 1;

        // keep reading until there is no readable socket
        while (readable) {

            // reset readable
            readable = 0;

            // prepare fd_set
            FD_ZERO(&readfds);
            maxfd = -1;

            for (int i = 0; i < FD_SETSIZE; i++) {

                if (client[i].conn_socket != -1) {

                    FD_SET(client[i].conn_socket, &readfds);
                    if (client[i].conn_socket > maxfd) {
                        maxfd = client[i].conn_socket;
                    }
                }
            }

            // checks if we can receive from any socket
            select(maxfd + 1, &readfds, NULL, NULL, &tv);

            // receives message
            for (int i = 0; i < FD_SETSIZE; i++) {

                if (client[i].conn_socket == -1) continue;

                if (FD_ISSET(client[i].conn_socket, &readfds)) {

                    readable++;
                    recv(client[i].conn_socket, &pkg, dpkg_size, 0);
                    
                    if (strcmp(pkg.control_signal, SYS_HELLO) == 0) {

                        // set to active
                        pthread_mutex_lock(&mutex);
                        client[i].is_active = 1;
                        pthread_mutex_unlock(&mutex);

                    } else if (strcmp(pkg.control_signal, SYS_QUIT) == 0) {

                        // clear client info
                        pthread_mutex_lock(&mutex);
                        close(client[i].conn_socket);
                        client[i].conn_socket = -1;
                        pthread_mutex_unlock(&mutex);

                    }
                }
            }
        }

        // clear inactive client info
        for (int i = 0; i < FD_SETSIZE; i++) {

            if (client[i].conn_socket == -1) continue;

            if (client[i].is_active != 1) {

                // clear client info
                pthread_mutex_lock(&mutex);
                close(client[i].conn_socket);
                client[i].conn_socket = -1;
                pthread_mutex_unlock(&mutex);
            }
        }
    }
}

void *listening_socket_handler(void *param) {

    data_package *my_addr_pkg = (data_package *) param;

    int conn_fd;

    struct sockaddr_in client_addr;
    int sin_size;

    data_package pkg;
    int dpkg_size = sizeof(data_package);

    //* create socket
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        report_error();
        exit(0);
    }

    //* bind listen socket with listening address
    if (bind(listen_fd, (struct sockaddr *) &my_addr_pkg->address, sizeof(my_addr_pkg->address)) < 0) {
        report_error();
        exit(0);
    }

    //* set socket for listening
    if (listen(listen_fd, 10) < 0) {
        report_error();
        exit(0);
    }

    while (1) {

        sin_size = sizeof(struct sockaddr_in);

        // accept connection request
        if ((conn_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &sin_size)) < 0) {
            report_error();
            exit(0);
        }

        // receive new client's init package
        recv(conn_fd, &pkg, dpkg_size, 0);

        // store new client info
        for (int i = 0; i < FD_SETSIZE; i++) {
            
            if (client[i].conn_socket == -1) {

                pthread_mutex_lock(&mutex);

                client[i].conn_socket = conn_fd;
                copy_sockaddr_in(&client[i].listen_address, &pkg.address);

                pthread_mutex_unlock(&mutex);

                break;
            }
        }
    }

    return NULL;
}

void *active_signal_sender() {

    data_package active_signal_pkg;
    int dpkg_size = sizeof(data_package);

    //* prepare package
    memset(&active_signal_pkg, '\0', sizeof(active_signal_pkg));
    strcpy(active_signal_pkg.control_signal, SYS_HELLO);

    //* send
    while (1) {

        // send to server
        send(serv_conn_fd, &active_signal_pkg, sizeof(active_signal_pkg), 0);

        // send to other clients
        for (int i = 0; i < FD_SETSIZE; i++) {

            if (client[i].conn_socket != -1) {

                if (send(client[i].conn_socket, &active_signal_pkg, sizeof(active_signal_pkg), 0) < 0) {

                    // clear client info
                    pthread_mutex_lock(&mutex);
                    close(client[i].conn_socket);
                    client[i].conn_socket = -1;
                    pthread_mutex_unlock(&mutex);
                }
            }
        }

        // sleep for 10s
        sleep(10);
    }

    return NULL;
}

void copy_sockaddr_in(struct sockaddr_in *dest, struct sockaddr_in *src) {
    memset(dest, '\0', sizeof(*dest));
    dest->sin_family = src->sin_family;
    dest->sin_port = src->sin_port;
    dest->sin_addr.s_addr = src->sin_addr.s_addr;
}
