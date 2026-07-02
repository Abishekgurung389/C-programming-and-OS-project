#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/auth.sock"

int main() {
    char username[64];
    char password[64];
    char message[130];
    char result[32];

    printf("=== Login System ===\n");
    printf("Username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;

    printf("Password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0;

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("[Frontend] Cannot connect to backend! Is backend running?\n");
        return 1;
    }

    snprintf(message, sizeof(message), "%s:%s", username, password);
    write(sock, message, strlen(message));

    read(sock, result, sizeof(result));
    printf("[Frontend] Backend says: %s\n", result);

    close(sock);
    return 0;
}
