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
#define PORT_NUMBER 10001

#define SYS_HELLO "#SYS_HELLO#"
#define SYS_ADDR "#SYS_ADDR#"
#define SYS_DONE "#SYS_DONE#"
#define SYS_QUIT "#SYS_QUIT#"

typedef struct client_info_t {
    int conn_socket; // server's socket connected to client
    struct sockaddr_in listen_address; // the address of listening socket of client
    int is_active; // indicate that client is active or not
} client_info;

typedef struct data_package_t {
    char control_signal[BUFF_SIZE];
    struct sockaddr_in address;
} data_package;

client_info client[FD_SETSIZE];

pthread_mutex_t mutex; /* uses to lock shared memory */

/**
 * every 30s, check if there are "hello" messages from clients.
 * if the "hello" message from any client is not received,
 * this client will be considered inactive.
*/
void *active_client_handler();

/**
 * copy struct sockaddr_in from src to dest
*/
void copy_sockaddr_in(struct sockaddr_in *dest, struct sockaddr_in *src);

int main(int argc, char *argv[]) {

    //* variables declaration
    int listen_fd; /* server's listening socket */
    int conn_fd; /* socket connected to client */

    struct sockaddr_in server_addr; /* server address */
    struct sockaddr_in client_addr; /* connected client address */
    struct sockaddr_in client_listen_addr; /* address of listening socket of client */
    int sin_size; /* stores address size */
    
    data_package pkg;
    int dpkg_size = sizeof(data_package);

    //* create socket
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        report_error();
        exit(0);
    }

    //* fill server adddress
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT_NUMBER);

    //* bind listen socket with server address
    if (bind(listen_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        report_error();
        exit(0);
    }

    //* init client info array
    for (int i = 0; i < FD_SETSIZE; i++) {
        client[i].conn_socket = -1;
    }

    //* init mutex
    pthread_mutex_init(&mutex, NULL);

    //* set socket for listening to client connection request
    if (listen(listen_fd, 10) < 0) {
        report_error();
        exit(0);
    }

    //* create new thread to handle client's active status
    pthread_t new_thread;
    if (pthread_create(&new_thread, NULL, active_client_handler, NULL) < 0) {
        report_error();
        exit(0);
    }
    pthread_detach(new_thread);

    //* notification
    printf("Server is running...\n");

    //* communicate with clients
    while (1) {

        sin_size = sizeof(struct sockaddr_in);

        // accept connection request
        if ((conn_fd = accept(listen_fd, (struct sockaddr *) &client_addr, &sin_size)) < 0) {
            report_error();
            exit(0);
        }

        // receive new client's init package
        recv(conn_fd, &pkg, dpkg_size, 0);
        copy_sockaddr_in(&client_listen_addr, &pkg.address);

        // send current client list to new client
        memset(pkg.control_signal, '\0', sizeof(pkg.control_signal));
        strcpy(pkg.control_signal, SYS_ADDR);

        for (int i = 0; i < FD_SETSIZE; i++) {

            if (client[i].conn_socket != -1) {

                copy_sockaddr_in(&pkg.address, &client[i].listen_address);
                send(conn_fd, &pkg, sizeof(pkg), 0);
            }
        }

        memset(pkg.control_signal, '\0', sizeof(pkg.control_signal));
        strcpy(pkg.control_signal, SYS_DONE);
        send(conn_fd, &pkg, sizeof(pkg), 0);

        // store new client info
        for (int i = 0; i < FD_SETSIZE; i++) {
            
            if (client[i].conn_socket == -1) {

                // store
                pthread_mutex_lock(&mutex);

                client[i].conn_socket = conn_fd;
                copy_sockaddr_in(&client[i].listen_address, &client_listen_addr);

                pthread_mutex_unlock(&mutex);

                // noti
                printf("connected [%s:%d]\n",
                    inet_ntoa(client[i].listen_address.sin_addr), ntohs(client[i].listen_address.sin_port));

                break;
            }
        }
    }

    //* close listening socket
    close(listen_fd);

    //* destroy mutex
    pthread_mutex_destroy(&mutex);

    //* notification
    printf("Server stopped!\n");

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

                    printf("%s:%d:%s\n",
                        inet_ntoa(client[i].listen_address.sin_addr), ntohs(client[i].listen_address.sin_port), pkg.control_signal);
                    
                    if (strcmp(pkg.control_signal, SYS_HELLO) == 0) {

                        // set to active
                        pthread_mutex_lock(&mutex);
                        client[i].is_active = 1;
                        pthread_mutex_unlock(&mutex);

                    } else if (strcmp(pkg.control_signal, SYS_QUIT) == 0) {

                        // noti
                        printf("disconnected [%s:%d]\n",
                            inet_ntoa(client[i].listen_address.sin_addr), ntohs(client[i].listen_address.sin_port));

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

                // noti
                printf("disconnected [%s:%d]\n",
                    inet_ntoa(client[i].listen_address.sin_addr), ntohs(client[i].listen_address.sin_port));

                // clear client info
                pthread_mutex_lock(&mutex);
                close(client[i].conn_socket);
                client[i].conn_socket = -1;
                pthread_mutex_unlock(&mutex);
            }
        }
    }
}

void copy_sockaddr_in(struct sockaddr_in *dest, struct sockaddr_in *src) {
    memset(dest, '\0', sizeof(*dest));
    dest->sin_family = src->sin_family;
    dest->sin_port = src->sin_port;
    dest->sin_addr.s_addr = src->sin_addr.s_addr;
}
