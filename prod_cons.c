#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <unistd.h>
#include <unistd.h>

#define BUFFER_SIZE 5
#define PRODUCERS_COUNT 5
#define CONSUMERS_COUNT 9
#define HOW_MANY_ITERATIONS 1024

typedef struct
{
    int buf[BUFFER_SIZE];  
    int in;               
    int out;
    int full_sid;
    int empty_sid;
    struct sembuf full_sem;
    struct sembuf empty_sem;

} sbuf_t;

sbuf_t shared;

int full_sid;
    int empty_sid;
    struct sembuf full_sem;
    struct sembuf empty_sem;

void printBuffer() {
    int i = 0;
    printf("[");
    for(i = 0; i < BUFFER_SIZE; i++) {
        printf("%d, ", shared.buf[i]);
        fflush(stdout);
    }
    printf("]\n");
    fflush(stdout);
}

void printSemVals() {
    int empty = semctl(empty_sid, 0, GETVAL, 0); 
    int full = semctl(full_sid, 0, GETVAL, 0); 
    printf("Empty: %d Full: %d\n", empty, full);
    fflush(stdout);
}

void *Producer(void *arg)
{
    int i, item, index;
    index = (int) arg;

    for (i=0; i < HOW_MANY_ITERATIONS; i++) {
        item = rand();

        // Try to acquire if there's free space on stack
        empty_sem.sem_op = -1;
        semop(empty_sid, &empty_sem, 1);

        shared.buf[shared.in] = item;
        printf("P%d produkuje %d ...\n", index, item);
        fflush(stdout);

        // Print current status
        printBuffer();
        printSemVals();

        // Put produced value to stack
        shared.in = (shared.in + 1) % BUFFER_SIZE;

        fflush(stdout);

        // Announce that value has been produced
        full_sem.sem_op = 1;
        semop(full_sid, &full_sem, 1);

        printSemVals();

        usleep(600000);
    }
    return NULL;
}

void *Consumer(void *arg)
{
    int i, item, index;

    index = (int)arg;
    for (i=HOW_MANY_ITERATIONS; i > 0; i--) {

        printSemVals();
    
        // Try to acquire
        full_sem.sem_op = -1;
        semop(full_sid, &full_sem, 1);

        item=shared.buf[shared.out];
        shared.buf[shared.out] = -1;
        shared.out = (shared.out + 1) % BUFFER_SIZE;

        printf("K%d zjada  %d ...\n", index, item);
        fflush(stdout);

        // Print current algoritm status
        printBuffer();

        // Announce that there's one more empty place
        empty_sem.sem_op = 1;
        semop(empty_sid, &empty_sem, 1);

        printSemVals();

        usleep(500000);
    }
    return NULL;
}

int main() {
    pthread_t idP, idC;
    int i;

    // Initialize semaphores
    empty_sid = semget(0x123, 1, 0600 | IPC_CREAT);
    full_sid = semget(0x200, 1, 0600 | IPC_CREAT);

    // Error handling
    if (shared.empty_sid == -1 || shared.full_sid == -1) {
        perror("semget");
        exit(1); 
    }

    empty_sem.sem_num = 0;
    empty_sem.sem_flg = 0;
    full_sem.sem_flg = 0;
    full_sem.sem_num = 0;

    // Reset semaphores to initial state
    semctl(empty_sid, 0, SETVAL, BUFFER_SIZE);
    semctl(full_sid, 0, SETVAL, 0);

    printSemVals();

    // Create producers and consumers on separate threads
    for (i = 0; i < PRODUCERS_COUNT; i++) {
        usleep(300000);
        pthread_create(&idP, NULL, Producer, (int*)i);
    }

    for(i = 0; i < CONSUMERS_COUNT; i++) {
        usleep(740000);
        pthread_create(&idC, NULL, Consumer, (void*)i);
    }

    pthread_exit(NULL);
}