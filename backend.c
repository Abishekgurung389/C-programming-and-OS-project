#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/types.h>

#define SOCKET_PATH "/tmp/auth.sock"
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
    char message[130];
    char username[64];
    char password[64];

    printf("[Backend] Starting with UID: %d\n", getuid());
    printf("[Backend] Effective UID: %d\n", geteuid());

    int server = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    unlink(SOCKET_PATH);
    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 5);

    printf("[Backend] Waiting for frontend to connect...\n");

    int client = accept(server, NULL, NULL);
    read(client, message, sizeof(message));
    printf("[Backend] Received credentials\n");

    char *colon = strchr(message, ':');
    *colon = '\0';
    strcpy(username, message);
    strcpy(password, colon + 1);

    int result = validate(username, password);

    // Drop privileges permanently
    uid_t myuid = getuid();
    setresuid(myuid, myuid, myuid);

    printf("[Backend] After drop - UID: %d\n", getuid());
    printf("[Backend] After drop - Effective UID: %d\n", geteuid());

    if (result) {
        write(client, "ACCESS GRANTED", 14);
        printf("[Backend] Result: ACCESS GRANTED\n");
    } else {
        write(client, "ACCESS DENIED", 13);
        printf("[Backend] Result: ACCESS DENIED\n");
    }

    close(client);
    close(server);
    unlink(SOCKET_PATH);
    return 0;
}
