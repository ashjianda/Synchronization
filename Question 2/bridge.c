#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define MAX_PEOPLE_ON_BRIDGE 5

int current_north = 0, current_south = 0;
int first_north = 1;
int first_south = 1;

pthread_mutex_t bridge_mutex;
sem_t north_sem, south_sem, bridge_capacity_sem;

void* north(void* arg) {
    sem_wait(&north_sem); 
    sem_wait(&bridge_capacity_sem); 
    pthread_mutex_lock(&bridge_mutex);

    current_north++;
    printf("person is passing to north %d\n", current_north);
    if (first_north) {
        printf("first person crossing the bridge to north\n");
        first_north = 0;
    }

    pthread_mutex_unlock(&bridge_mutex);
    sem_post(&bridge_capacity_sem); 
    sem_post(&north_sem);
    pthread_exit(0);
}

void* south(void* arg) {
    sem_wait(&south_sem);
    sem_wait(&bridge_capacity_sem);
    pthread_mutex_lock(&bridge_mutex);

    current_south++;
    printf("person is passing to south %d\n", current_south);
    if (first_south) {
        printf("first person crossing the bridge to south\n");
        first_south = 0;
    }

    pthread_mutex_unlock(&bridge_mutex);
    sem_post(&bridge_capacity_sem);
    sem_post(&south_sem);
    pthread_exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./bridge <max_people_on_bridge>\n");
        exit(1);
    }

    int num_people = atoi(argv[1]); 
    pthread_t north_threads[num_people];
    pthread_t south_threads[num_people];

    pthread_mutex_init(&bridge_mutex, NULL);
    sem_init(&north_sem, 0, 1);
    sem_init(&south_sem, 0, 1);
    sem_init(&bridge_capacity_sem, 0, num_people);

    printf("Maximum number of people can cross the bridge: %d\n", num_people);

    for (int i = 0; i < num_people; i++) {
        pthread_create(&north_threads[i], NULL, north, NULL);
    }

    for (int i = 0; i < num_people; i++) {
        pthread_create(&south_threads[i], NULL, south, NULL);
    }

    for (int i = 0; i < num_people; i++) {
        pthread_join(north_threads[i], NULL);
        pthread_join(south_threads[i], NULL);
    }

    pthread_mutex_destroy(&bridge_mutex);
    sem_destroy(&north_sem);
    sem_destroy(&south_sem);
    sem_destroy(&bridge_capacity_sem);

    return 0;
}