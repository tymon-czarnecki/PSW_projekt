#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "list.h"

void *producer(void *arg) {
    TList *list = (TList *)arg;
    for (int i = 0; i < 5; i++) {
        int *data = (int *)malloc(sizeof(int));
        *data = i;
        printf("Producent: dodaję %d\n", *data);
        putItem(list, data);
    }
    return NULL;
}

void *consumer(void *arg) {
    TList *list = (TList *)arg;
    for (int i = 0; i < 5; i++) {
        int *data = (int *)getItem(list);
        if (data) {
            printf("Konsument: pobrałem %d\n", *data);
            free(data);
        } else {
            printf("Konsument: lista jest zniszczona.\n");
        }
    }
    return NULL;
}

int main() {
    TList *list = createList(3); // Maksymalny rozmiar listy to 3

    pthread_t producerThread, consumerThread;

    // Tworzenie wątków
    pthread_create(&producerThread, NULL, producer, list);
    pthread_create(&consumerThread, NULL, consumer, list);

    // Oczekiwanie na zakończenie wątków
    pthread_join(producerThread, NULL);
    pthread_join(consumerThread, NULL);

    destroyList(list);

    return 0;
}