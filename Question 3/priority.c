#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define NUMBER_OF_READERS_AND_WRITERS 3

sem_t *sem_reader, *sem_writer;
int *write_count;
int *shared_value;

void reader() {
    sem_wait(sem_reader);
    
    printf("Reader Process Id %d Reading %d\n", getpid(), *shared_value);
    
    sem_post(sem_reader);
    exit(0);
}

void writer() {
    (*write_count)++;
    if (*write_count == 1) {
        sem_wait(sem_reader);
    }

    if(fork() == 0) {
        reader();
    }
    sem_wait(sem_writer);
    
    *shared_value *= 2;
    printf("Writer Process Id %d Writing %d\n", getpid(), *shared_value);

    (*write_count)--;
    if (*write_count == 0) {
        sem_post(sem_reader);
    }
    
    sem_post(sem_writer);
    exit(0);
}

int main() {
    int write_count_shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    int shared_value_shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    int sem_reader_shmid = shmget(IPC_PRIVATE, sizeof(sem_t), IPC_CREAT | 0666);
    int sem_writer_shmid = shmget(IPC_PRIVATE, sizeof(sem_t), IPC_CREAT | 0666);
    
    if (write_count_shmid == -1 || shared_value_shmid == -1 || 
        sem_reader_shmid == -1 || sem_writer_shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    write_count = (int *)shmat(write_count_shmid, NULL, 0);
    shared_value = (int *)shmat(shared_value_shmid, NULL, 0);
    sem_reader = (sem_t *)shmat(sem_reader_shmid, NULL, 0);
    sem_writer = (sem_t *)shmat(sem_writer_shmid, NULL, 0);

    if (write_count == (void *)-1 || shared_value == (void *)-1 || 
        sem_reader == (void *)-1 || sem_writer == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    *write_count = 0;
    *shared_value = 1;
    *writer_waiting = 0;

    sem_init(sem_reader, 1, 1);
    sem_init(sem_writer, 1, 1);

    printf("Total number of readers and writers: %d\n", NUMBER_OF_READERS_AND_WRITERS);

    for (int i = 0; i < NUMBER_OF_READERS_AND_WRITERS; i++) {
        if (fork() > 0)
            writer();
    }

    for (int i = 0; i < NUMBER_OF_READERS_AND_WRITERS; i++) {
        wait(NULL);
    }

    sem_destroy(sem_reader);
    sem_destroy(sem_writer);

    shmdt(write_count);
    shmdt(shared_value);
    shmdt(sem_reader);
    shmdt(sem_writer);

    shmctl(write_count_shmid, IPC_RMID, NULL);
    shmctl(shared_value_shmid, IPC_RMID, NULL);
    shmctl(sem_reader_shmid, IPC_RMID, NULL);
    shmctl(sem_writer_shmid, IPC_RMID, NULL);

    return 0;
}