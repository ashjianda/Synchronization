#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 1000

int *input_buffer, *output_buffer;
int input_index = 0, copy_index = 0, output_index = 0, consume_index = 0;
int buffer_size;

pthread_mutex_t input_mutex, output_mutex;
sem_t producer_done, copier_done;

void *producer(void *arg) {
    int value = rand() % 100;
    pthread_mutex_lock(&input_mutex);
    input_buffer[input_index++] = value;
    printf("Inserted %d\n", value);
    pthread_mutex_unlock(&input_mutex);
    sem_post(&producer_done);
    pthread_exit(0);
}

void *copier(void *arg) {
    sem_wait(&producer_done);
    
    pthread_mutex_lock(&input_mutex);
    int value = input_buffer[copy_index++];
    pthread_mutex_unlock(&input_mutex);
    
    pthread_mutex_lock(&output_mutex);
    output_buffer[output_index++] = value;
    pthread_mutex_unlock(&output_mutex);
    sem_post(&copier_done);
    pthread_exit(0);
}

void *consumer(void *arg) {
    sem_wait(&copier_done);
    
    pthread_mutex_lock(&output_mutex);
    int value = output_buffer[consume_index++];
    pthread_mutex_unlock(&output_mutex);
    printf("Consumed %d\n", value);    
    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./semaphores <buffer_size>\n");
        exit(1);
    }

    buffer_size = atoi(argv[1]);
    if (buffer_size > MAX_BUFFER_SIZE || buffer_size <= 0) {
        printf("Invalid buffer size. Exiting.\n");
        exit(1);
    }

    input_buffer = (int *)malloc(buffer_size * sizeof(int));
    output_buffer = (int *)malloc(buffer_size * sizeof(int));

    pthread_mutex_init(&input_mutex, NULL);
    pthread_mutex_init(&output_mutex, NULL);
    sem_init(&producer_done, 0, 0);
    sem_init(&copier_done, 0, 0);

    int num_threads = buffer_size;
    pthread_t producers[num_threads], copiers[num_threads], consumers[num_threads];
    printf("The buffer size is %d\n", buffer_size);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&producers[i], NULL, producer, NULL);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&copiers[i], NULL, copier, NULL);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&consumers[i], NULL, consumer, NULL);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(producers[i], NULL);
        pthread_join(copiers[i], NULL);
        pthread_join(consumers[i], NULL);
    }

    pthread_mutex_destroy(&input_mutex);
    pthread_mutex_destroy(&output_mutex);
    sem_destroy(&producer_done);
    sem_destroy(&copier_done);

    free(input_buffer);
    free(output_buffer);

    return 0;
}
