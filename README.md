# ST5039CMD - Programming and Operating Systems
## C Programming and OS Project

## Project Overview
This project demonstrates operating system concepts including:
- Process isolation using fork() and execve()
- Inter-process communication via UNIX domain sockets
- Privilege management using setresuid()
- Concurrent monitoring using pthreads
- Signal-based process termination

## Task 1: Privilege Separated Authentication System

### What it does
A secure login system split into two separate processes:
- frontend.c takes username and password from user, sends to backend
- backend.c validates credentials, drops privileges, wipes memory

### How to compile
gcc frontend.c -o frontend
gcc backend.c -o backend

### How to run
./frontend
Type username: abishek
Type password: 1234

### Features
- Process isolation with fork() and execve()
- Secure IPC via UNIX domain socket
- Privilege dropping with setresuid()
- Runtime verification via /proc/self/status
- Secure memory wiping after authentication
- Attack resistance - rejects malformed requests

## Task 2: User Space Malware Analysis Sandbox

### What it does
A sandbox that runs untrusted programs safely and kills
them if they misbehave.

### How to compile
gcc sandbox.c -o sandbox -lpthread
gcc infinite_loop.c -o infinite_loop
gcc cpu_hog.c -o cpu_hog
gcc normal_program.c -o normal_program

### How to run
Test 1 - program that runs forever:
./sandbox ./infinite_loop

Test 2 - program that eats CPU:
./sandbox ./cpu_hog

Test 3 - normal program that finishes on its own:
./sandbox ./normal_program

### Files
- sandbox.c - Main sandbox controller
- infinite_loop.c - Test binary that runs forever
- cpu_hog.c - Test binary that uses 100% CPU
- normal_program.c - Test binary that finishes normally
- sandbox.log - Log file of all sandbox events

### Features
- Parent-child process isolation
- 3 concurrent monitoring threads
- Time limit enforcement 5 seconds
- CPU usage monitoring
- Memory limit 50MB using setrlimit()
- Complete logging to sandbox.log
- SIGKILL based termination

## Author
- Name: Abishek Gurung
- Module: ST5039CMD Programming and Operating Systems
- Institution: Softwarica College of IT and E-Commerce
- Collaborating University: Coventry University# C-programming-and-OS-project
