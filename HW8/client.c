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

#include <sys/utsname.h>    // library to get kernel info

#include "util.h"

#define BUFF_SIZE 2048
#define NEXT_LINE "\n"

#define SYS_BYE "#SYS_BYE#"

int main(int argc, char *argv[]) {

    //* variables declaration
    char server_ip[20]; /* server ip address */
    int port_number; /* server opened port */

    int clientfd; /* client socket descriptor */
    struct sockaddr_in server_addr; /* server's address */

    char buff[BUFF_SIZE];
    int recv_bytes, send_bytes;

    struct utsname os_info; /* store OS info */

    int choice;
    int connected;
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
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port_number);

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

    connected = 1;
    printf("Client started!\n");

    //* communicate with server
    while (connected) {
        
        // menu
        printf("-------------Menu-------------\n");
        printf("1. Gui thong tin cau hinh\n");
        printf("2. Gui thong tin he dieu hanh\n");
        printf("3. Gui thong diep bat ky\n");
        printf("4. Thoat\n");
        printf("Your choice: ");
        scanf("%d", &choice);
        clear_stdin_buff();

        switch (choice) {
            case 1:

                memset(buff, '\0', sizeof(buff));
                strcpy(buff, "-------------------CPU-------------------");
                send(clientfd, buff, strlen(buff), 0);
                send(clientfd, NEXT_LINE, strlen(NEXT_LINE), 0);

                fp = popen("lscpu", "r");
                while (fgets(buff, sizeof(buff) - 1, fp) != NULL) {
                    send(clientfd, buff, strlen(buff), 0);
                }
                pclose(fp);

                memset(buff, '\0', sizeof(buff));
                strcpy(buff, "-------------------RAM-------------------");
                send(clientfd, buff, strlen(buff), 0);
                send(clientfd, NEXT_LINE, strlen(NEXT_LINE), 0);

                fp = popen("free -h", "r");
                while (fgets(buff, sizeof(buff) - 1, fp) != NULL) {
                    send(clientfd, buff, strlen(buff), 0);
                }
                pclose(fp);

                break;
            case 2:

                memset(buff, '\0', sizeof(buff));
                strcpy(buff, "-------------------KERNEL-------------------");
                send(clientfd, buff, strlen(buff), 0);
                send(clientfd, NEXT_LINE, strlen(NEXT_LINE), 0);

                uname(&os_info);
                send(clientfd, os_info.sysname, strlen(os_info.sysname), 0);
                send(clientfd, NEXT_LINE, strlen(NEXT_LINE), 0);
                send(clientfd, os_info.release, strlen(os_info.release), 0);
                send(clientfd, NEXT_LINE, strlen(NEXT_LINE), 0);
                send(clientfd, os_info.version, strlen(os_info.version), 0);
                send(clientfd, NEXT_LINE, strlen(NEXT_LINE), 0);
                send(clientfd, os_info.machine, strlen(os_info.machine), 0);
                send(clientfd, NEXT_LINE, strlen(NEXT_LINE), 0);

                memset(buff, '\0', sizeof(buff));
                strcpy(buff, "-------------------OS-------------------");
                send(clientfd, buff, strlen(buff), 0);
                send(clientfd, NEXT_LINE, strlen(NEXT_LINE), 0);

                fp = popen("cat /etc/os-release", "r");
                while (fgets(buff, sizeof(buff) - 1, fp) != NULL) {
                    send(clientfd, buff, strlen(buff), 0);
                }
                pclose(fp);

                break;
            case 3:

                memset(buff, '\0', sizeof(buff));
                strcpy(buff, "-------------------MESSAGE-------------------");
                send(clientfd, buff, strlen(buff), 0);
                send(clientfd, NEXT_LINE, strlen(NEXT_LINE), 0);

                memset(buff, '\0', sizeof(buff));
                printf("Message: ");
                fgets(buff, BUFF_SIZE, stdin);
                buff[strlen(buff) - 1] = '\0';
                send(clientfd, buff, strlen(buff), 0);
                send(clientfd, NEXT_LINE, strlen(NEXT_LINE), 0);

                break;
            case 4:
                send(clientfd, SYS_BYE, strlen(SYS_BYE), 0);
                connected = 0;
                break;
        }
    }

    //* close socket
    close(clientfd);

    printf("Client stopped!\n");

    return 0;
}