#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>

sem_t *mutex, *wrt;
int *read_count;       // To count the number of readers
int *shared_value;     // Shared integer variable

// Writer function for the parent process
void writer(int num_writes) {
    for (int i = 0; i < num_writes; i++) {
        sem_wait(wrt); // Only one writer can access the shared value at a time
        
        *shared_value *= 2;
        printf("Writer Process Id %d Writing %d\n", getpid(), *shared_value);
        
        sem_post(wrt); // Release access to the shared variable for the next writer or reader
        sleep(1); // Optional: add delay to simulate processing
    }
}

// Reader function for child processes
void reader() {
    sem_wait(mutex); // Control access to the read_count variable
    
    (*read_count)++;
    if (*read_count == 1) {
        sem_wait(wrt); // Block writers if this is the first reader
    }
    sem_post(mutex);
    
    // Reading the shared variable
    printf("Reader's Process Id %d Reading %d\n", getpid(), *shared_value);
    
    sem_wait(mutex);
    (*read_count)--;
    if (*read_count == 0) {
        sem_post(wrt); // Release writer access if this is the last reader
    }
    sem_post(mutex);
    
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: ./priority <number_of_readers> <number_of_writes>\n");
        exit(1);
    }
    int num_readers = atoi(argv[1]);
    int num_writes = atoi(argv[2]);

    // Set up shared memory using shm API
    int read_count_shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    int shared_value_shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    int mutex_shmid = shmget(IPC_PRIVATE, sizeof(sem_t), IPC_CREAT | 0666);
    int wrt_shmid = shmget(IPC_PRIVATE, sizeof(sem_t), IPC_CREAT | 0666);
    
    if (read_count_shmid == -1 || shared_value_shmid == -1 || mutex_shmid == -1 || wrt_shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    // Attach shared memory
    read_count = (int *)shmat(read_count_shmid, NULL, 0);
    shared_value = (int *)shmat(shared_value_shmid, NULL, 0);
    mutex = (sem_t *)shmat(mutex_shmid, NULL, 0);
    wrt = (sem_t *)shmat(wrt_shmid, NULL, 0);

    if (read_count == (void *)-1 || shared_value == (void *)-1 || mutex == (void *)-1 || wrt == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize shared variables
    *read_count = 0;
    *shared_value = 1;

    // Initialize semaphores
    sem_init(mutex, 1, 1);
    sem_init(wrt, 1, 1);

    printf("Total number of readers and writers: %d\n", num_readers + num_writes);

    // Create reader processes (children)
    for (int i = 0; i < num_readers; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            reader();
        }
    }

    // Parent acts as writer
    writer(num_writes);

    // Wait for all child (reader) processes to complete
    for (int i = 0; i < num_readers; i++) {
        wait(NULL);
    }

    // Destroy semaphores
    sem_destroy(mutex);
    sem_destroy(wrt);

    // Detach and remove shared memory
    shmdt(read_count);
    shmdt(shared_value);
    shmdt(mutex);
    shmdt(wrt);

    shmctl(read_count_shmid, IPC_RMID, NULL);
    shmctl(shared_value_shmid, IPC_RMID, NULL);
    shmctl(mutex_shmid, IPC_RMID, NULL);
    shmctl(wrt_shmid, IPC_RMID, NULL);

    return 0;
}
