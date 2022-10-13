#ifndef __SERVER_H__
#define __SERVER_H__

#define MAX_LENGTH 30
#define MAX_CONSECUTIVE_FAIL 3

typedef struct Account_ {
    char username[MAX_LENGTH + 1];
    char password[MAX_LENGTH + 1];
    int status;
    int consecutiveFailedSignIn;
    int isSignedIn;
    struct Account_ *next;
} Account;

Account* start();
Account* findAccount(Account *list, char *username);
int isActiveAccount(Account *list, char *username);
Account* registerAccount(Account *list, char *username, char *password);
void signIn(Account *list, char *username, char *password);
void searchForAccount(Account *list, char *username);
void signOut(Account *list, char *username);
int userSignedIn(Account *list);

#endif
