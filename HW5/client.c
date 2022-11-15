#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "util.h"

#define BUFF_SIZE 2048
#define OK "SYS_MSG_OK"
#define ERR "SYS_MSG_ERR"
#define STOP "SYS_MSG_STOP"
#define EMPTY_STR "SYS_MSG_EMPTY_STR"
#define SEND_MSG_CMD "SYS_MSG_SEND_MSG_CMD"
#define SEND_FILE_CMD "SYS_MSG_SEND_FILE_CMD"
#define SENDING_FILE "SYS_MSG_SENDING_FILE"
#define DONE_SENDING_FILE "SYS_MSG_DONE_SENDING_FILE"
#define BLANK_LINE "SYS_MSG_BLANK_LINE"
#define STOP_PROG_CMD "SYS_MSG_STOP_PROG_CMD"

int main(int argc, char *argv[]) {

    //* variables declaration
    char server_ip[20]; /* server ip address */
    int port_number; /* server opened port */

    int clientfd; /* client socket descriptor */
    struct sockaddr_in serv_addr; /* server's address */

    char buff[BUFF_SIZE];
    int recv_bytes, send_bytes;

    int choice;

    char file_path[1000];
    FILE *fp;

    //* init
    if (argc == 3) {
        strcpy(server_ip, argv[1]);
        port_number = str_to_number(argv[2]);
    } else {
        printf("Usage: ./client <server_ip> <port_number>\n");
        exit(0);
    }

    //* fill server address info
    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);
    serv_addr.sin_port = htons(port_number);

    printf("Client started!\n");

    //* communicate with server
    while (1) {

        // create socket
        if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            report_error();
            exit(0);
        }

        // connect to server
        if (connect(clientfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            report_error(0);
            exit(0);
        }
        
        // menu
        printf("------------Menu------------\n");
        printf("1. Gui xau bat ky\n");
        printf("2. Gui noi dung mot file\n");
        printf("3. Thoat\n");
        printf("Your choice: ");
        scanf("%d", &choice);
        clear_stdin_buff();

        // send message service
        if (choice == 1) {

            // send send-message-command to server
            send(clientfd, SEND_MSG_CMD, strlen(SEND_MSG_CMD), 0);

            while (1) {

                // input message
                printf("Message: ");
                fgets(buff, BUFF_SIZE, stdin);
                buff[strlen(buff) - 1] = '\0';

                // send message
                if (strlen(buff) == 0) {
                    // input message is empty, send stop request & stop service
                    send(clientfd, STOP, strlen(STOP), 0);
                    break;
                } else {
                    // input message is not empty, send message to server
                    send(clientfd, buff, strlen(buff), 0);
                }

                // receive result status
                recv_bytes = recv(clientfd, buff, BUFF_SIZE, 0);
                buff[recv_bytes] = '\0';
                
                if (strcmp(buff, OK) == 0) {
                    // split successfully
                    
                    // send sending allowance message
                    send(clientfd, OK, strlen(OK), 0);

                    // receive first result
                    recv_bytes = recv(clientfd, buff, BUFF_SIZE, 0);
                    buff[recv_bytes] = '\0';
                    if (strcmp(buff, EMPTY_STR) != 0) {
                        printf("%s\n", buff);
                    }
                    
                    // send sending allowance message
                    send(clientfd, OK, strlen(OK), 0);

                    // receive second result
                    recv_bytes = recv(clientfd, buff, BUFF_SIZE, 0);
                    buff[recv_bytes] = '\0';
                    if (strcmp(buff, EMPTY_STR) != 0) {
                        printf("%s\n", buff);
                    }
                } else {
                    // split fail
                    printf("Error!\n");
                }
            }
        }

        // send file service
        if (choice == 2) {

            // send send-file-command to server
            send(clientfd, SEND_FILE_CMD, strlen(SEND_FILE_CMD), 0);

            // input file path
            printf("File path: ");
            scanf("%s", file_path);
            clear_stdin_buff();
            
            // send file content
            fp = fopen(file_path, "r");

            if (fp) {

                while (!feof(fp)) {

                    // send sending-file status
                    send(clientfd, SENDING_FILE, strlen(SENDING_FILE), 0);

                    // wait for sending allowance
                    recv_bytes = recv(clientfd, buff, BUFF_SIZE, 0);
                    buff[recv_bytes] = '\0';

                    // get file content and send to server
                    if (strcmp(buff, OK) == 0) {

                        // get content
                        memset(buff, '\0', sizeof(buff));
                        fread(buff, BUFF_SIZE, 1, fp);

                        // send to server
                        if (strlen(buff) > 0) {
                            send(clientfd, buff, strlen(buff), 0);
                        } else {
                            send(clientfd, BLANK_LINE, strlen(BLANK_LINE), 0);
                        }

                        // wait for allowance to continue
                        recv(clientfd, buff, BUFF_SIZE, 0);
                    }
                }
                // sending is done, send done message to server
                send(clientfd, DONE_SENDING_FILE, strlen(DONE_SENDING_FILE), 0);

                fclose(fp);
            } else {
                send(clientfd, DONE_SENDING_FILE, strlen(DONE_SENDING_FILE), 0);
                printf("Can not open file!\n");
            }
        }

        // stop program --> send stop program message, close socket & stop
        if (choice == 3) {
            send(clientfd, STOP_PROG_CMD, strlen(STOP_PROG_CMD), 0);
            close(clientfd);
            break;
        }

        // close socket
        close(clientfd);
    }

    printf("Client stopped!\n");
    return 0;
}