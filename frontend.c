#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

#define SOCKET_PATH "/tmp/auth.sock"

int main() {
    char username[64];
    char password[64];
    char message[130];
    char result[32];

    // Step 1: fork() - create a child process
    pid_t pid = fork();

    if (pid < 0) {
        printf("[Frontend] fork() failed!\n");
        return 1;
    }

    if (pid == 0) {
        // I am the CHILD - become backend using execve()
        printf("[Frontend] Child process launching backend...\n");
        
        // Small delay so backend socket is ready
        sleep(1);
        
        char *args[] = {"./backend", NULL};
        char *env[] = {NULL};
        
        execve("./backend", args, env);
        
        // If execve fails
        printf("[Frontend] execve() failed!\n");
        exit(1);
    }

    // I am the PARENT - I am the frontend
    printf("[Frontend] Parent PID: %d\n", getpid());
    printf("[Frontend] Backend PID: %d\n", pid);

    // Wait for backend to start
    sleep(2);

    // Step 2: Get user input
    printf("\n=== Login System ===\n");
    printf("Username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;

    printf("Password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0;

    // Step 3: Connect to backend via UNIX socket
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("[Frontend] Cannot connect to backend!\n");
        return 1;
    }

    // Step 4: Send credentials
    snprintf(message, sizeof(message), "%s:%s", username, password);
    write(sock, message, strlen(message));

    // Step 5: Get result
    memset(result, 0, sizeof(result));
    read(sock, result, sizeof(result));
    printf("[Frontend] Backend says: %s\n", result);

    close(sock);

    // Wait for backend to finish
    wait(NULL);
    printf("[Frontend] Done.\n");

    return 0;
}
