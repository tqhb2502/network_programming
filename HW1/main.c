#include <stdio.h>
#include "server.h"

int main() {

    int userChoice;
    int quit = 0;

    Account *accList;
    char username[MAX_LENGTH + 1];
    char password[MAX_LENGTH + 1];

    // Đọc dữ liệu tài khoản
    accList = start();

    // Bắt đầu chương trình quản lý tài khoản người dùng
    while (1) {

        // Menu
        printf("------------------------------\n");
        printf("USER MANAGEMENT PROGRAM\n");
        printf("------------------------------\n");
        printf("1. Register\n");
        printf("2. Sign in\n");
        printf("3. Search\n");
        printf("4. Sign out\n");
        printf("Your choice (1-4, other to quit): ");
        scanf("%d", &userChoice);

        // Xử lý lựa chọn
        switch (userChoice) {

        case 1:
            printf("Username: ");
            scanf("%s", username);
            // Kiểm tra tài khoản tồn tại
            if (findAccount(accList, username)) {
                printf("Account existed\n");
                continue;
            }

            printf("Password: ");
            scanf("%s", password);

            accList = registerAccount(accList, username, password);
            break;

        case 2:
            printf("Username: ");
            scanf("%s", username);
            // Kiểm tra tài khoản tồn tại
            if (!findAccount(accList, username)) {
                printf("Cannot find account\n");
                continue;
            }
            // Kiểm tra tài khoản hoạt động
            if (!isActiveAccount(accList, username)) {
                printf("Account is blocked\n");
                continue;
            }

            printf("Password: ");
            scanf("%s", password);

            signIn(accList, username, password);
            break;

        case 3:
            // Kiểm tra đăng nhập
            if (!userSignedIn(accList)) {
                printf("You are not signed in\n");
                continue;
            }

            printf("Username: ");
            scanf("%s", username);
            searchForAccount(accList, username);
            break;

        case 4:
            printf("Username: ");
            scanf("%s", username);
            signOut(accList, username);
            break;

        default:
            quit = 1;
        }

        // Kiểm tra điều kiện thoát
        if (quit) {
            break;
        }
    }

    return 0;
}
