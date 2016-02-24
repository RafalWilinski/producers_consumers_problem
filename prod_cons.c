#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <unistd.h>
#include <unistd.h>

#define BUFFER_SIZE 5
#define PRODUCERS_COUNT 5
#define CONSUMERS_COUNT 4
#define HOW_MANY_ITERATIONS 102400000
#define PROD_SPEED 45
#define CONS_SPEED 34

int full_sid;
int empty_sid;
int full_arr_sid;
int empty_arr_sid;

struct sembuf full_sem;
struct sembuf empty_sem;
struct sembuf full_arr_sem;
struct sembuf empty_arr_sem;

int items[BUFFER_SIZE] = {-1, -1, -1, -1, -1};
int emptyPositions[BUFFER_SIZE] = {0, 1, 2, 3, 4};
int fullPositions[BUFFER_SIZE] = {-1, -1, -1, -1, -1};
int emptyPositionsIn = 0, emptyPositionsOut = 0, fullPositionsIn = 0, fullPositionsOut = 0;

void printBuffer() {
    printf("Buffer: {");
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        printf("%d, ", items[i]);
        fflush(stdout);
    }
    printf("}\n");
    fflush(stdout);
    usleep(10);
}

void printFreePositions() {
    printf("Free: {");
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        printf("%d, ", emptyPositions[i]);
        fflush(stdout);
    }
    printf("} In: %d, Out: %d\n", emptyPositionsIn, emptyPositionsOut);
    fflush(stdout);
    usleep(10);
}

void printWorkingPositions() {
    printf("Working: {");
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        printf("%d, ", fullPositions[i]);
        fflush(stdout);
    }
    printf("} In: %d, Out: %d\n", fullPositionsIn, fullPositionsOut);
    fflush(stdout);
    usleep(10);
}

void printSemVals() {
    int empty = semctl(empty_sid, 0, GETVAL, 0);
    int full = semctl(full_sid, 0, GETVAL, 0);
    printf("Empty: %d Full: %d\n", empty, full);
    fflush(stdout);
    usleep(10);
}

void printAllInfo() {
    printSemVals();
    printBuffer();
    printFreePositions();
    printWorkingPositions();
}

void *Producer(void *arg)
{
    int i, item, bufferIndex;

    for (i=0; i < HOW_MANY_ITERATIONS; i++) {
        printf("Zgloszenie producenta.\n");

        empty_sem.sem_op = -1;
        semop(empty_sid, &empty_sem, 1);

        printf("Rozpoczynam produkcje. Wolne miejsca do produkcji: \n");
        printFreePositions();

        // Zablokuj tablice empty
        empty_arr_sem.sem_op = -1;
        semop(empty_arr_sid, &empty_arr_sem, 1);

        // Rezerwuj miejsce w buforze z tablicy free[out]
        bufferIndex = emptyPositions[emptyPositionsIn];
        emptyPositions[emptyPositionsIn] = -1;
        emptyPositionsIn = (emptyPositionsIn + 1) % BUFFER_SIZE;

        // Odblokuj tablice empty
        empty_arr_sem.sem_op = 1;
        semop(empty_arr_sid, &empty_arr_sem, 1);

        printf("Miejsce do ktorego zostanie wyprodukowany przedmiot: %d. Rozpoczynam produkcje...\n", bufferIndex);

        // Zacznij wytwarzac przedmiot
        usleep((useconds_t) (10000000 * (rand() / (RAND_MAX + 0.5))) / PROD_SPEED);
        item = rand();

        printf("Produkcja zakonczona. Produkt: %d\n", item);

        // Dodaj element do bufora
        items[bufferIndex]= item;
        printf("Stan bufora po dodaniu: \n");
        printBuffer();

        // Zablokuj tablice full
        full_arr_sem.sem_op = -1;
        semop(full_arr_sid, &full_arr_sem, 1);

        // Dodaj element do tablicy working[in]
        fullPositions[fullPositionsOut] = bufferIndex;
        fullPositionsOut = (fullPositionsOut + 1) % BUFFER_SIZE;

        // Odblokuj tablice full
        full_arr_sem.sem_op = 1;
        semop(full_arr_sid, &full_arr_sem, 1);

        // Powiadom o tym, ze przedmiot do konsumpcji jest juz dostępny
        full_sem.sem_op = 1;
        semop(full_sid, &full_sem, 1);

        printf("Koniec produkcji...\n");

        printAllInfo();

        usleep((useconds_t) (10000000 * (rand() / (RAND_MAX + 0.5))) / PROD_SPEED);
    }
    return NULL;
}

void *Consumer(void *arg)
{
    int i, item, bufferIndex;

    for (i=HOW_MANY_ITERATIONS; i > 0; i--) {

        printf("Zgloszenie konsumenta.\n");
        full_sem.sem_op = -1;
        semop(full_sid, &full_sem, 1);

        printf("Rozpoczynam konsumowanie. Co można zjeść: \n");
        printWorkingPositions();

        // Zablokuj tablice full
        full_arr_sem.sem_op = -1;
        semop(full_arr_sid, &full_arr_sem, 1);

        // Rezerwuj miejsce w buforze z tablicy free[out]
        bufferIndex = fullPositions[fullPositionsIn];
        fullPositionsIn = (fullPositionsIn + 1) % BUFFER_SIZE;

        // Odblokuj tablice empty
        full_arr_sem.sem_op = 1;
        semop(full_arr_sid, &full_arr_sem, 1);

        printf("Miejsce z ktorego zostanie zjedzony przedmiot: %d. Rozpoczynam konsumpcje...\n", bufferIndex);

        // Zacznij konsumowac przedmiot
        usleep((useconds_t) (10000000 * (rand() / (RAND_MAX + 0.5 ))) / CONS_SPEED);
        item = items[bufferIndex];

        // Usun przedmiot z bufora
        items[bufferIndex] = -1;

        if(item == -1 || bufferIndex == -1) printf("\n******************\n****** BŁĄD ******\n*****************\n");
        printf("Konsumpcja zakonczona. Zjedzony: %d\n", item);

        // Zablokuj tablice empty
        empty_arr_sem.sem_op = -1;
        semop(empty_arr_sid, &empty_arr_sem, 1);

        // Dodaj element do tablicy empty[in]
        emptyPositions[emptyPositionsOut] = bufferIndex;
        emptyPositionsOut = (emptyPositionsOut + 1) % BUFFER_SIZE;

        // Odblokuj tablice full
        empty_arr_sem.sem_op = 1;
        semop(empty_arr_sid, &empty_arr_sem, 1);

        // Po zjedzeniu powiedz, ze jest o jeden wiecej wolny
        empty_sem.sem_op = 1;
        semop(empty_sid, &empty_sem, 1);

        printf("Po konsupmcji.\n");

        printAllInfo();
        usleep((useconds_t) (10000000 * (rand() / (RAND_MAX + 0.5))) / CONS_SPEED);
    }
    return NULL;
}

int main() {
    pthread_t idP, idC;
    int i;

    // Initialize semaphores
    empty_sid = semget(0x123, 1, 0600 | IPC_CREAT);
    full_sid = semget(0x124, 1, 0600 | IPC_CREAT);
    empty_arr_sid = semget(0x125, 1, 0600 | IPC_CREAT);
    full_arr_sid = semget(0x126, 1, 0600 | IPC_CREAT);

    // Error handling
    if (empty_sid == -1 || full_sid == -1 || empty_arr_sid == -1 || full_arr_sid == -1) {
        perror("semget");
        exit(1);
    }

    empty_sem.sem_num = 0;
    empty_sem.sem_flg = 0;
    full_sem.sem_flg = 0;
    full_sem.sem_num = 0;
    empty_arr_sem.sem_num = 0;
    empty_arr_sem.sem_flg = 0;
    full_arr_sem.sem_flg = 0;
    full_arr_sem.sem_num = 0;

    // Reset semaphores to initial state
    semctl(empty_sid, 0, SETVAL, BUFFER_SIZE);
    semctl(full_sid, 0, SETVAL, 0);
    semctl(empty_arr_sid, 0, SETVAL, 1);
    semctl(full_arr_sid, 0, SETVAL, 1);

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