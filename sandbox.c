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
#include <sys/resource.h>  // NEW: for setrlimit

#define MAX_TIME    5
#define MAX_CPU     500
#define MAX_MEMORY  50 * 1024 * 1024  // NEW: 50MB memory limit

pid_t child_pid = -1;
int child_killed = 0;
FILE *logfile = NULL;

void log_event(const char *msg) {
    time_t now = time(NULL);
    char *t = ctime(&now);
    t[strlen(t)-1] = '\0';
    printf("[SANDBOX LOG %s] %s\n", t, msg);
    if (logfile) {
        fprintf(logfile, "[SANDBOX LOG %s] %s\n", t, msg);
        fflush(logfile);
    }
}

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
    char *tok = strtok(buf, " ");
    for (i = 1; i < 14; i++) tok = strtok(NULL, " ");
    utime = atol(tok);
    tok = strtok(NULL, " ");
    stime = atol(tok);
    return (float)(utime + stime);
}

// NEW: get memory usage of child from /proc
long get_memory_usage(pid_t pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            fclose(f);
            long kb;
            sscanf(line, "VmRSS: %ld", &kb);
            return kb * 1024; // convert to bytes
        }
    }
    fclose(f);
    return -1;
}

void *time_monitor(void *arg) {
    log_event("Time monitor thread started");
    int seconds = 0;
    while (seconds < MAX_TIME) {
        sleep(1);
        seconds++;
        char msg[64];
        snprintf(msg, sizeof(msg), "Child running for %d seconds", seconds);
        log_event(msg);
        if (waitpid(child_pid, NULL, WNOHANG) != 0) {
            log_event("Child already finished");
            return NULL;
        }
    }
    if (!child_killed) {
        child_killed = 1;
        log_event("TIME LIMIT EXCEEDED - Killing child process!");
        kill(child_pid, SIGKILL);
    }
    return NULL;
}

void *cpu_monitor(void *arg) {
    log_event("CPU monitor thread started");
    float prev = get_cpu_usage(child_pid);
    sleep(1);
    while (!child_killed) {
        float curr = get_cpu_usage(child_pid);
        if (curr < 0) break;
        float diff = curr - prev;
        prev = curr;
        char msg[64];
        snprintf(msg, sizeof(msg), "CPU ticks: %.0f", diff);
        log_event(msg);
        if (diff > MAX_CPU) {
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

// NEW: Thread 3 - memory monitor
void *memory_monitor(void *arg) {
    log_event("Memory monitor thread started");
    while (!child_killed) {
        long mem = get_memory_usage(child_pid);
        if (mem < 0) break;

        char msg[64];
        snprintf(msg, sizeof(msg), "Memory usage: %ld KB", mem/1024);
        log_event(msg);

        if (mem > MAX_MEMORY) {
            if (!child_killed) {
                child_killed = 1;
                log_event("MEMORY LIMIT EXCEEDED - Killing child process!");
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
        printf("Usage: ./sandbox <program>\n");
        return 1;
    }

    logfile = fopen("sandbox.log", "a");
    if (!logfile) {
        printf("[Sandbox] Warning: cannot open log file\n");
    }

    printf("[Sandbox] Starting sandbox for: %s\n", argv[1]);
    log_event("Sandbox started");

    child_pid = fork();

    if (child_pid < 0) {
        printf("[Sandbox] fork() failed!\n");
        return 1;
    }

    if (child_pid == 0) {
        // NEW: set memory limit for child before execve
        struct rlimit mem_limit;
        mem_limit.rlim_cur = MAX_MEMORY;
        mem_limit.rlim_max = MAX_MEMORY;
        setrlimit(RLIMIT_AS, &mem_limit);
        log_event("Memory limit set for child");

        printf("[Sandbox] Child process starting untrusted binary...\n");
        char *args[] = {argv[1], NULL};
        char *env[] = {NULL};
        execve(argv[1], args, env);
        printf("[Sandbox] execve() failed!\n");
        exit(1);
    }

    printf("[Sandbox] Child PID: %d\n", child_pid);
    printf("[Sandbox] Max time: %d seconds\n", MAX_TIME);
    printf("[Sandbox] Max memory: %d MB\n", MAX_MEMORY/1024/1024);

    // Start THREE monitoring threads
    pthread_t time_thread, cpu_thread, mem_thread;
    pthread_create(&time_thread, NULL, time_monitor, NULL);
    pthread_create(&cpu_thread, NULL, cpu_monitor, NULL);
    pthread_create(&mem_thread, NULL, memory_monitor, NULL);  // NEW

    int status;
    waitpid(child_pid, &status, 0);

    if (child_killed) {
        log_event("RESULT: Child was forcefully terminated by sandbox");
    } else if (WIFEXITED(status)) {
        char msg[64];
        snprintf(msg, sizeof(msg), "RESULT: Child finished normally with exit code %d", 
                 WEXITSTATUS(status));
        log_event(msg);
    } else {
        log_event("RESULT: Child terminated abnormally");
    }

    pthread_join(time_thread, NULL);
    pthread_join(cpu_thread, NULL);
    pthread_join(mem_thread, NULL);  // NEW

    log_event("Sandbox finished");
    if (logfile) fclose(logfile);
    return 0;
}
