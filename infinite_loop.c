#include <stdio.h>
#include <unistd.h>

int main() {
    printf("[Untrusted] I am running forever...\n");
    while(1) {
        sleep(1);
        printf("[Untrusted] Still running...\n");
    }
    return 0;
}
