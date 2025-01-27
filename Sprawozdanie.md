

---
title:    Lista o ograniczonej pojemności
subtitle: Programowanie systemowe i współbieżne
author:   Tymon Czarnecki 160265 \<<tymon.czarnecki@student.put.poznan.pl>\>
date:     2025-01-23
lang:     pl-PL
---


Projekt jest dostępny w repozytorium pod adresem:  
<https://github.com/tymon-czarnecki/PSW_projekt>.


# Struktury danych

1. Elementy listy definiowane są strukturą `TNode`:
	```C
	struct TNode {
	    void *data;
	    struct TNode *next;
	};
	```
Zmienna `data` wskazuje na przechowywane dane, a `next` na następną strukturę budując łańcuch, który tworzy listę.

2. Struktura listy prezentuje się następująco:
    ```C
	struct TList {
	    struct TNode *head;
	    struct TNode *tail;
	    int size;
	    int maxSize;
	    int isDestroyed; //0 - nie jest, 1 - jest usuwana
	    int workersNum;
	    pthread_mutex_t lock;
	    pthread_cond_t canAdd;
	    pthread_cond_t canRemove;
	    pthread_cond_t canDestroy;
	};
	```
	
Zmienne `head` i `tail` wskazują na odpowiednio na pierwsze i ostatni z `TNode`, które budują listę. 
Wartość `isDestroyed` wskazuje czy lista jest w trakcie usuwania a wartość `workersNum` na liczbę wątków, które są oddelegowane do operacji na tej liście.

Mutex `lock` jest podstawowym narzędziem rezerwującym dostęp do listy dla tylko 1 wątku.
Warunki `canAdd`, `canRemove` oraz `canDestroyed` stanowią o tym czy można odpowiednio dodawać i usuwać elementy do Listy, i czy można już usunąć ją samą.

# Funkcje

Implementacja wykorzystuje dodatkowo 2 proste funkcje:

- **incrementWorkers(Tlist \*lst):**

	```C
	void incrementWorkers(TList *lst) {
	    pthread_mutex_lock(&lst->workersLock);
	    lst->workersNum++;
	    pthread_mutex_unlock(&lst->workersLock);
	}
	```

- **decrementWorker(TList \*lst):**
	```C
	void decrementWorkers(TList *lst) {
	    pthread_mutex_lock(&lst->workersLock);
	    lst->workersNum--;
	    pthread_mutex_unlock(&lst->workersLock);
	}
	```

Służą one do kontrolowania liczby wątków przydzielonych do pracy na jednej liście, są one wywoływane odpowiednio:
- `incrementWorker(TList *lst)` - gdy wątek uruchamia funkcje do manipulacji listą
- `decrementWorker(TList *lst)` - gdy wątek opuszcza funkcję

# Algorytm / dodatkowy opis

## 1. Ochrona przed zakleszczeniem

Algorytm unika zakleszczenia dzięki odpowiedniemu zarządzaniu muteksami i warunkami:

- **Kolejność blokad:** Muteksy są blokowane i zwalniane w przewidywalnej kolejności, co eliminuje możliwość wystąpienia cyklicznego oczekiwania na zasoby.
-   **Zwalnianie mutexów przed blokadą:** Przed rozpoczęciem długotrwałego oczekiwania na warunek `pthread_cond_wait` muteks jest zwalniany, co pozwala innym wątkom kontynuować pracę i zmieniać stan, zapobiegając zakleszczeniu.

## 2. Ochrona przed aktywnym oczekiwaniem

Algorytm unika aktywnego oczekiwania poprzez wykorzystanie zmiennych warunkowych `pthread_cond_t`

- **Efektywne oczekiwanie:** Zamiast aktywnie sprawdzać stan listy, wątki czekają w sposób pasywny za pomocą `pthread_cond_wait`. Mechanizm ten powoduje zawieszenie wątku do czasu zmiany warunku.

## 3. Ochrona przed głodzeniem

- **Sygnalizowanie:** Algorytm chroni przed głodzeniem dzięki mechanizmom `signal` i `broadcast`, które budzą oczekujące wątki, gdy tylko nastąpi w liście zmiana umożliwiająca im dalszą pracę.

## 4. Skrajne sytuacje

- **Pełna/pusta lista:** poleganie na warunkach `canAdd` i `canRemove` zapewnia bezpieczne przetwarzanie w przypadku braku możliwości wykonania operacji dodawania/usuwania elementu.

## 5. Usuwanie listy
Usunięcie listy w bezpieczny sposób, zapobiegając np. próbom dostępu do usuniętych muteksów, zapewniają flaga `isDestroyed`, warunek `canDestroy` i licznik `workersNum` we współpracy z odpowiednimi mechanizmami.

- **Fragment funkcji destroyList:**

	```C
	while (1) {
	        pthread_mutex_lock(&lst->workersLock);
	        if (lst->workersNum <= 1) {
	            pthread_mutex_unlock(&lst->workersLock);
	            break;
	        }
	        pthread_mutex_unlock(&lst->workersLock);
	        pthread_cond_wait(&lst->canDestroy, &lst->lock);
	    }
	```
zapewnia, że funkcja nie przejdzie dalej do usuwania elementów listy, dopóki istnieją jeszcze wątki oczekujące na zasoby tej listy.
    
Każda funkcja ma również zaimplementowany mechanizm, który prowadzi do zakończenia wątku przy wykryciu aktywnej flagi `isDestroyed` sygnalizującej o trwającym usuwaniu listy.

# Przykład użycia
## Problem konsumenta-producenta
Przykład implementacji problemu konsumenta-producenta przy użyciu listy o ograniczonej pojemności:

- **Implementacja:**
	```C
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
	```

- **Wyniki:**
	```
	Producent: dodaję 0
	Producent: dodaję 1
	Producent: dodaję 2
	Konsument: pobrałem 0
	Konsument: pobrałem 1
	Producent: dodaję 3
	Producent: dodaję 4
	Konsument: pobrałem 2
	Konsument: pobrałem 3
	Konsument: pobrałem 4
	```
