#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <unistd.h>
#include <unistd.h>

#define BUFFER_SIZE 5
#define PRODUCERS_COUNT 2
#define CONSUMERS_COUNT 2
#define HOW_MANY_ITERATIONS 1024

int full_sid;
int empty_sid;

struct sembuf full_sem;
struct sembuf empty_sem;

typedef struct node {
    struct node *next;
    int value;
};

struct node *root;

void addElement(int a) {
    if(root == NULL) {
        root = malloc(sizeof(struct node));
        root->next = NULL;
        root->value = a;
    } else {
        struct node *tmp = root;
        while(tmp->next != 0) {
            tmp = tmp->next;
        }

        struct node *newNode = malloc(sizeof(struct node));
        newNode->next = NULL;
        newNode->value = a;
        tmp->next = newNode; 
    }
}

int getFirstElementValue() {
    if(root != NULL) {
        int tmp = root->value;
        if(root->next != NULL) {
            root = root->next;
        } else {
            root = NULL;
        }

        return tmp;

    } else {
        printf("Buffer is empty!");
        return -1;
    }
}

void printBuffer() {
    int i = 0;
    struct node* tmp = root;

    printf("[");
    while(tmp) {
        printf("%d, ", tmp->value);
        tmp = tmp->next;
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

        printf("P%d produkuje %d ...\n", index, item);
        fflush(stdout);

        // Do some work
        usleep(1000000 * (rand() / (RAND_MAX+1.0)));

        // Announce that value has been produced
        full_sem.sem_op = 1;
        semop(full_sid, &full_sem, 1);

        printf("P%d wyprodukował %d.\n", index, item);
        addElement(item);
        printBuffer();
        printSemVals();
        fflush(stdout);
    }
    return NULL;
}

void *Consumer(void *arg)
{
    int i, item, index;

    index = (int)arg;
    for (i=HOW_MANY_ITERATIONS; i > 0; i--) {
    
        // Try to acquire
        full_sem.sem_op = -1;
        semop(full_sid, &full_sem, 1);

        item = getFirstElementValue();

        printf("K%d zjada  %d ...\n", index, item);
        fflush(stdout);

        // Print current algoritm status
        printBuffer();

        // Announce that there's one more empty place
        empty_sem.sem_op = 1;
        semop(empty_sid, &empty_sem, 1);

        printSemVals();

        usleep(1000000 * (rand() / (RAND_MAX+1.0)) );
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
    if (empty_sid == -1 || full_sid == -1) {
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