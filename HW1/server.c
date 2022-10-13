#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"

/**
 * Ghi dữ liệu từ linked-list vào file
*/
void writeToFile(Account *list) {

    FILE *fp = fopen("account.txt", "w");

    Account *acc = list;
    while (acc) {
        fprintf(fp, "%s %s %d\n", acc->username, acc->password, acc->status);
        acc = acc->next;
    }

    fclose(fp);
}

/**
 * Đọc dữ liệu từ file
 * Tạo ra 1 linked-list lưu trữ các tài khoản
 * Trả về 1 con trỏ đến tài khoản đầu tiên
*/
Account* start() {

    FILE *fp = fopen("account.txt", "r");

    Account *list = NULL;
    Account *prevAcc, *acc;

    char username[MAX_LENGTH + 1];
    char password[MAX_LENGTH + 1];
    int status;

    while (!feof(fp)) {
        // Đọc dữ liệu
        fscanf(fp, "%s", username);
        fscanf(fp, "%s", password);
        fscanf(fp, "%d", &status);

        // Kiểm tra dòng trống
        if (strlen(username) == 0) {
            continue;
        }

        // Lưu dữ liệu vào bộ nhớ
        acc = (Account *) malloc(sizeof(Account));
        strcpy(acc->username, username);
        strcpy(acc->password, password);
        acc->status = status;
        acc->consecutiveFailedSignIn = 0;
        acc->isSignedIn = 0;

        // Thêm vào danh sách
        if (list == NULL) {
            list = acc;
            prevAcc = acc;
        } else {
            prevAcc->next = acc;
            prevAcc = acc;
        }

        // Reset biến lưu dữ liệu
        strcpy(username, "");
        strcpy(password, "");
    }

    fclose(fp);

    return list;
}

/**
 * Tìm kiếm tài khoản trong danh sách
 * - Tìm thấy: trả về tài khoản đó
 * - Không tìm thấy: trả về NULL
*/
Account* findAccount(Account *list, char *username) {

    Account *acc = list;

    while (acc) {
        if (strcmp(acc->username, username) == 0) {
            return acc;
        }
        acc = acc->next;
    }

    return NULL;
}

/**
 * Kiểm tra tài khoản hoạt động
 * - Hoạt động: trả về 1
 * - Bị khóa: trả về 0
*/
int isActiveAccount(Account *list, char *username) {
    Account *acc = findAccount(list, username);
    if (acc) {
        return acc->status;
    }
    return 1;
}

/**
 * Đăng ký tài khoản mới
 * Trả về 1 con trỏ đến tài khoản đầu trong danh sách các tài khoản
*/
Account* registerAccount(Account *list, char *username, char *password) {

    // Tạo tài khoản mới
    Account *newAccout = (Account *) malloc(sizeof(Account));
    strcpy(newAccout->username, username);
    strcpy(newAccout->password, password);
    newAccout->status = 1;
    newAccout->consecutiveFailedSignIn = 0;
    newAccout->isSignedIn = 0;

    // Thêm tài khoản vào danh sách
    if (list == NULL) {
        list = newAccout;
    } else {

        Account *lastAccount = list;
        while (lastAccount->next) {
            lastAccount = lastAccount->next;
        }

        lastAccount->next = newAccout;  
    }

    // Lưu tài khoản vào file lưu trữ
    writeToFile(list);

    // In thông báo thành công
    printf("Successful registration\n");

    return list;
}

/**
 * Đăng nhập
*/
void signIn(Account *list, char *username, char *password) {

    // Tìm tài khoản tương ứng
    Account *account = findAccount(list, username);

    // Kiểm tra mật khẩu
    if (strcmp(account->password, password) != 0) {

        account->consecutiveFailedSignIn++;

        // Đăng nhập thất bại quá 3 lần
        if (account->consecutiveFailedSignIn > MAX_CONSECUTIVE_FAIL) {
            account->status = 0;
            writeToFile(list);
            printf("Password is incorrect. Account is blocked\n");
        } else {
            printf("Password is incorrect\n");
        }
    } else {
        account->isSignedIn = 1;
        printf("Hello %s\n", username);
    }
}

/**
 * Tìm kiếm
*/
void searchForAccount(Account *list, char *username) {

    // Tìm tài khoản tương ứng
    Account *acc = findAccount(list, username);

    // Nếu tài khoản tồn tại
    if (acc) {
        if (acc->status) {
            printf("Account is active\n");
        } else {
            printf("Account is blocked\n");
        }
    } else {
        printf("Cannot find account\n");
    }
}

/**
 * Đăng xuất
*/
void signOut(Account *list, char *username) {

    // Tìm tài khoản tương ứng
    Account *acc = findAccount(list, username);

    // Nếu tài khoản tồn tại
    if (acc) {
        if (acc->isSignedIn) {
            acc->isSignedIn = 0;
            printf("Goodbye %s\n", username);
        } else {
            printf("Account is not sign in\n");
        }
    } else {
        printf("Cannot find account\n");
    }
}

/**
 * Người dùng đã đăng nhập hay chưa
 * - Ít nhất 1 tài khoản: trả về 1
 * - Chưa đăng nhập: trả về 0
*/
int userSignedIn(Account *list) {
    Account *acc = list;
    while (acc) {
        if (acc->isSignedIn) return 1;
        acc = acc->next;
    }
    return 0;
}