#include <stdio.h>
#include <string.h>

#define CORRECT_USER "abishek"
#define CORRECT_PASS "1234"

int validate(char *username, char *password) {
    if (strcmp(username, CORRECT_USER) == 0 &&
        strcmp(password, CORRECT_PASS) == 0) {
        return 1;
    }
    return 0;
}

int main() {
    char username[64];
    char password[64];

    printf("[Backend] Enter username to validate: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;

    printf("[Backend] Enter password to validate: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0;

    if (validate(username, password)) {
        printf("[Backend] Result: ACCESS GRANTED\n");
    } else {
        printf("[Backend] Result: ACCESS DENIED\n");
    }

    return 0;
}
