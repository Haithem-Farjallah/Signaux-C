#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#define NUM_CHILDREN 4

sem_t *semaphore;

void signal_handler(int signal) {
    if (signal == SIGUSR1) {
        printf("Child process %d received SIGUSR1: Starting task\n", getpid());
    } else if (signal == SIGUSR2) {
        printf("Child process %d received SIGUSR2\n", getpid());
    } else if (signal == SIGTERM) {
        printf("Child process %d received SIGTERM: Terminating\n", getpid());
    }
}

void child_process() {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    sem_wait(semaphore);

    printf("Child process %d: Initialized and waiting for start signal\n", getpid());
    fflush(stdout);

    // Wait for SIGUSR1 to start the task
    pause();

    // Simulate complex task
    printf("Child process %d: Performing a complex task...\n", getpid());
    fflush(stdout);
    sleep(5);  // Simulate the time taken by the complex task

    // Send confirmation signal to parent
    kill(getppid(), SIGUSR1);

    printf("Child process %d: Task completed and confirmation sent to parent\n", getpid());
    fflush(stdout);

    exit(0);
}

int main() {
    pid_t pid[NUM_CHILDREN];

    semaphore = sem_open("/semaphore", O_CREAT, 0644, 0);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    printf("Parent process: Initialization complete\n");
    fflush(stdout);

    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid[i] = fork();
        if (pid[i] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid[i] == 0) {
            child_process();
        }
    }

    printf("Parent process: Waiting for all children to be ready\n");
    fflush(stdout);

    // Release semaphore to indicate children are ready
    for (int i = 0; i < NUM_CHILDREN; i++) {
        sem_post(semaphore);
    }

    printf("Parent process: Signaling all children to start their tasks\n");
    fflush(stdout);

    // Send start signal to children
    for (int i = 0; i < NUM_CHILDREN; i++) {
        kill(pid[i], SIGUSR1);
    }

    // Wait for children to finish and handle their confirmation signals
    for (int i = 0; i < NUM_CHILDREN; i++) {
        wait(NULL);
        printf("Parent process: Child process %d has completed its task\n", pid[i]);
        fflush(stdout);
    }

    printf("Parent process: All child processes have completed their tasks\n");
    fflush(stdout);

    sem_close(semaphore);
    sem_unlink("/semaphore");

    printf("Parent process: Cleanup complete and exiting\n");
    fflush(stdout);

    return 0;
}
