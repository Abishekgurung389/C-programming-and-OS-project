#include <stdio.h>
#include <unistd.h>

int main() {
    printf("[Untrusted] I am a good program!\n");
    printf("[Untrusted] Doing some work...\n");
    sleep(2);
    printf("[Untrusted] I finished my work. Goodbye!\n");
    return 0;
}
