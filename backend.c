#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>

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

// Secure memory wipe
void secure_wipe(void *ptr, size_t len) {
    volatile unsigned char *p = ptr;
    while (len--) {
        *p++ = 0;
    }
}

// Attack resistance - check if message is valid
int is_valid_request(char *message) {
    // Must contain exactly one colon
    int colon_count = 0;
    for (int i = 0; message[i] != '\0'; i++) {
        if (message[i] == ':') colon_count++;
    }
    if (colon_count != 1) {
        printf("[Backend] ATTACK DETECTED: malformed request rejected!\n");
        return 0;
    }
    // Must not be empty
    if (strlen(message) < 3) {
        printf("[Backend] ATTACK DETECTED: request too short!\n");
        return 0;
    }
    return 1;
}

// Runtime check using /proc/self/status
void check_privileges_proc() {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Uid:", 4) == 0 ||
            strncmp(line, "Gid:", 4) == 0) {
            printf("[Backend] /proc check - %s", line);
        }
    }
    fclose(f);
}

int main() {
    char message[130];
    char username[64];
    char password[64];

    printf("[Backend] Starting with UID: %d\n", getuid());
    printf("[Backend] Effective UID: %d\n", geteuid());

    // Lock memory
    mlock(message, sizeof(message));
    mlock(password, sizeof(password));

    int server = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    unlink(SOCKET_PATH);
    bind(server, (struct sockaddr*)&addr, sizeof(addr));
    listen(server, 5);

    printf("[Backend] Waiting for frontend to connect...\n");

    int client = accept(server, NULL, NULL);
    
    memset(message, 0, sizeof(message));
    read(client, message, sizeof(message) - 1);
    printf("[Backend] Received credentials\n");

    // Attack resistance check
    if (!is_valid_request(message)) {
        write(client, "ACCESS DENIED", 13);
        close(client);
        close(server);
        unlink(SOCKET_PATH);
        return 1;
    }

    // Split username:password
    char *colon = strchr(message, ':');
    *colon = '\0';
    strcpy(username, message);
    strcpy(password, colon + 1);

    // Validate
    int result = validate(username, password);

    // Wipe sensitive data immediately
    secure_wipe(password, sizeof(password));
    secure_wipe(message, sizeof(message));
    printf("[Backend] Sensitive data wiped from memory\n");

    // Drop privileges permanently
    uid_t myuid = getuid();
    setresuid(myuid, myuid, myuid);

    // Verify via /proc
    printf("[Backend] Verifying privilege drop via /proc:\n");
    check_privileges_proc();

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
