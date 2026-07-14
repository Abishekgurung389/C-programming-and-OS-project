#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

#define MAX_TIME 5      // kill after 5 seconds
#define MAX_CPU  80     // kill if CPU usage over 80%

// Shared state between threads
pid_t child_pid = -1;
int child_killed = 0;

// Log function
void log_event(const char *msg) {
    time_t now = time(NULL);
    char *t = ctime(&now);
    t[strlen(t)-1] = '\0'; // remove newline
    printf("[SANDBOX LOG %s] %s\n", t, msg);
}

// Get CPU usage of child from /proc
float get_cpu_usage(pid_t pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    long utime, stime;
    int i;
    char buf[512];
    fgets(buf, sizeof(buf), f);
    fclose(f);

    // Fields in /proc/pid/stat
    char *tok = strtok(buf, " ");
    for (i = 1; i < 14; i++) tok = strtok(NULL, " ");
    utime = atol(tok);
    tok = strtok(NULL, " ");
    stime = atol(tok);

    return (float)(utime + stime);
}

// Thread 1: watches time
void *time_monitor(void *arg) {
    log_event("Time monitor thread started");
    
    int seconds = 0;
    while (seconds < MAX_TIME) {
        sleep(1);
        seconds++;
        
        char msg[64];
        snprintf(msg, sizeof(msg), "Child running for %d seconds", seconds);
        log_event(msg);

        // Check if child already finished
        if (waitpid(child_pid, NULL, WNOHANG) != 0) {
            log_event("Child already finished");
            return NULL;
        }
    }

    // Time limit exceeded - kill child
    if (!child_killed) {
        child_killed = 1;
        log_event("TIME LIMIT EXCEEDED - Killing child process!");
        kill(child_pid, SIGKILL);
    }
    return NULL;
}

// Thread 2: watches CPU
void *cpu_monitor(void *arg) {
    log_event("CPU monitor thread started");

    float prev = get_cpu_usage(child_pid);
    sleep(1);

    while (!child_killed) {
        float curr = get_cpu_usage(child_pid);
        if (curr < 0) break; // process gone

        float diff = curr - prev;
        prev = curr;

        char msg[64];
        snprintf(msg, sizeof(msg), "CPU ticks used: %.0f", diff);
        log_event(msg);

        if (diff > 500) { // high CPU usage
            if (!child_killed) {
                child_killed = 1;
                log_event("CPU LIMIT EXCEEDED - Killing child process!");
                kill(child_pid, SIGKILL);
            }
            break;
        }
        sleep(1);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./sandbox <program_to_run>\n");
        printf("Example: ./sandbox ./infinite_loop\n");
        return 1;
    }

    printf("[Sandbox] Starting sandbox for: %s\n", argv[1]);
    log_event("Sandbox started");

    // fork() - create child process
    child_pid = fork();

    if (child_pid < 0) {
        printf("[Sandbox] fork() failed!\n");
        return 1;
    }

    if (child_pid == 0) {
        // I am the CHILD - run the untrusted program
        printf("[Sandbox] Child process starting untrusted binary...\n");
        
        char *args[] = {argv[1], NULL};
        char *env[] = {NULL};
        execve(argv[1], args, env);
        
        // If execve fails
        printf("[Sandbox] execve() failed!\n");
        exit(1);
    }

    // I am the PARENT - monitor the child
    printf("[Sandbox] Child PID: %d\n", child_pid);
    printf("[Sandbox] Monitoring started. Max time: %d seconds\n", MAX_TIME);

    // Start monitoring threads
    pthread_t time_thread, cpu_thread;
    pthread_create(&time_thread, NULL, time_monitor, NULL);
    pthread_create(&cpu_thread, NULL, cpu_monitor, NULL);

    // Wait for child to finish
    int status;
    waitpid(child_pid, &status, 0);

    if (child_killed) {
        log_event("Child was forcefully terminated by sandbox");
    } else {
        log_event("Child finished on its own");
    }

    // Wait for threads to finish
    pthread_join(time_thread, NULL);
    pthread_join(cpu_thread, NULL);

    log_event("Sandbox finished");
    return 0;
}
