#include <stdio.h>
#include <string.h>

int main() {
    char username[64];
    char password[64];

    printf("=== Login System ===\n");
    printf("Username: ");
    fgets(username, sizeof(username), stdin);

    printf("Password: ");
    fgets(password, sizeof(password), stdin);

    username[strcspn(username, "\n")] = 0;
    password[strcspn(password, "\n")] = 0;

    printf("\n[Frontend] Received username: %s\n", username);
    printf("[Frontend] Password received, sending to backend...\n");

    return 0;
}
