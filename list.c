#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "list.h"


TList *createList(int s) {
    TList *list = (TList *)malloc(sizeof(TList));

    list->head = list->tail = NULL;
    list->size = 0;
    list->maxSize = s;
    list->isDestroyed = 0;
    list->workersNum = 0;

    pthread_mutex_init(&list->lock, NULL);
    pthread_cond_init(&list->canAdd, NULL);
    pthread_cond_init(&list->canRemove, NULL);
    pthread_cond_init (&list->canDestroy, NULL);
    return list;
}

void destroyList(TList *lst) {
    lst->workersNum++;
    
    pthread_mutex_lock(&lst->lock);
    if(lst->isDestroyed == 0){
    lst->isDestroyed = 1;
    
    lst->size = 1;
    lst->maxSize = 2;
    pthread_cond_broadcast(&lst->canAdd);
    pthread_cond_broadcast(&lst->canRemove);
    
    } else {
        lst->workersNum--;
        pthread_mutex_unlock(&lst->lock);
        pthread_cond_signal(&lst->canDestroy);
        return;
    }

    while(lst->workersNum > 1){
        pthread_cond_wait(&lst->canDestroy, &lst->lock);
    }

    struct TNode *current = lst->head;
    while (current) {
        struct TNode *temp = current;
        current = current->next;

        free(temp->data);
        free(temp); 
    }


    pthread_mutex_unlock(&lst->lock);
    pthread_mutex_destroy(&lst->lock);
    pthread_cond_destroy(&lst->canAdd);
    pthread_cond_destroy(&lst->canRemove);
    pthread_cond_destroy(&lst->canDestroy);

    free(lst);
}


void putItem(TList *lst, void *itm) {
    lst->workersNum ++;
    pthread_mutex_lock(&lst->lock);

    while (lst->size >= lst->maxSize) {
        pthread_cond_wait(&lst->canAdd, &lst->lock);
    }
    
    if(lst->isDestroyed == 1){
        lst->workersNum--;
        pthread_mutex_unlock(&lst->lock);
        pthread_cond_signal(&lst->canDestroy);
        return;
        }

    struct TNode *newNode = (struct TNode *)malloc(sizeof(struct TNode));
    newNode->data = itm;
    newNode->next = NULL;

    if (!lst->head) {
        lst->head = lst->tail = newNode;
    } else {
        lst->tail->next = newNode;
        lst->tail = newNode;
    }

    lst->size++;
    pthread_cond_broadcast(&lst->canRemove);
    lst->workersNum--;
    pthread_mutex_unlock(&lst->lock);
}

void *getItem(TList *lst) {
    lst->workersNum ++;
    pthread_mutex_lock(&lst->lock);

    while (lst->size == 0) {
        pthread_cond_wait(&lst->canRemove, &lst->lock);
        if(lst->isDestroyed == 1){
            lst->workersNum--;
            pthread_mutex_unlock(&lst->lock);
            pthread_cond_signal(&lst->canDestroy);
            return NULL;
        }
    }

    struct TNode *temp = lst->head;
    void *data = temp->data;
    lst->head = lst->head->next;

    if (!lst->head) {
        lst->tail = NULL;
    }

    free(temp);
    lst->size--;
    
    if (lst->size < lst->maxSize) pthread_cond_broadcast(&lst->canAdd);
    pthread_mutex_unlock(&lst->lock);
    
    lst->workersNum--;
    return data;
}

void *popItem(TList *lst) {
    lst->workersNum ++;
    pthread_mutex_lock(&lst->lock);

    while (lst->size == 0) {
        pthread_cond_wait(&lst->canRemove, &lst->lock);
    }
    
    if(lst->isDestroyed == 1){
        lst->workersNum--;
        pthread_mutex_unlock(&lst->lock);
        pthread_cond_signal(&lst->canDestroy);
        return NULL;
    }

    struct TNode *prev = NULL;
    struct TNode *current = lst->head;

    while (current->next) {
        prev = current;
        current = current->next;
    }

    void *data = current->data;

    if (prev) {
        prev->next = NULL;
        lst->tail = prev;
    } else {
        lst->head = lst->tail = NULL;
    }

    free(current);
    lst->size--;

    pthread_cond_signal(&lst->canAdd);
    pthread_mutex_unlock(&lst->lock);
    
    lst->workersNum--;
    return data;
}

int removeItem(TList *lst, void *itm) {
    lst->workersNum ++;
    pthread_mutex_lock(&lst->lock);

    if(lst->isDestroyed == 1){
        lst->workersNum--;
        pthread_mutex_unlock(&lst->lock);
        pthread_cond_signal(&lst->canDestroy);
        return 0;
    }

    struct TNode *prev = NULL;
    struct TNode *current = lst->head;

    while (current) {
        if (current->data == itm) {
            if (prev) {
                prev->next = current->next;
            } else {
                lst->head = current->next;
            }

            if (!current->next) {
                lst->tail = prev;
            }

            free(current);
            lst->size--;

            pthread_cond_signal(&lst->canAdd);
            pthread_mutex_unlock(&lst->lock);

            return 1;
        }

        prev = current;
        current = current->next;
    }

    pthread_mutex_unlock(&lst->lock);
    lst->workersNum--;
    return 0;
}

int getCount(TList *lst) {
    lst->workersNum ++;
    pthread_mutex_lock(&lst->lock);
    
    if(lst->isDestroyed == 1){
        lst->workersNum--;
        pthread_mutex_unlock(&lst->lock);
        pthread_cond_signal(&lst->canDestroy);
        return -1;
    }    
    
    int count = lst->size;
    pthread_mutex_unlock(&lst->lock);
    lst->workersNum--;
    return count;
}

void setMaxSize(TList *lst, int s) {
    lst->workersNum ++;
    pthread_mutex_lock(&lst->lock);
    
    if(lst->isDestroyed == 1){
        lst->workersNum--;
        pthread_mutex_unlock(&lst->lock);
        pthread_cond_signal(&lst->canDestroy);
        return;
    }    
    
    lst->maxSize = s;
    pthread_cond_broadcast(&lst->canAdd);
    lst->workersNum--;
    pthread_mutex_unlock(&lst->lock);
}

void appendItems(TList *lst, TList *lst2) {
    lst->workersNum ++;
    lst2->workersNum ++;    
    pthread_mutex_lock(&lst->lock);
    pthread_mutex_lock(&lst2->lock);

    if((lst->isDestroyed == 1) || (lst2->isDestroyed == 1)){
        if(lst->isDestroyed == 1){
            lst->workersNum--;
            pthread_mutex_unlock(&lst->lock);
            pthread_cond_signal(&lst->canDestroy);
        }
        if(lst2->isDestroyed == 1){
            lst2->workersNum--;
            pthread_mutex_unlock(&lst2->lock);
            pthread_cond_signal(&lst2->canDestroy);
        }
        return;
    }

    if (lst2->head) {
        if (lst->tail) {
            lst->tail->next = lst2->head;
        } else {
            lst->head = lst2->head;
        }

        lst->tail = lst2->tail;
        lst->size += lst2->size;
    }

    lst2->head = lst2->tail = NULL;
    lst2->size = 0;

    pthread_cond_broadcast(&lst->canRemove);
    lst->workersNum--;
    lst2->workersNum--;
    pthread_mutex_unlock(&lst2->lock);
    pthread_mutex_unlock(&lst->lock);
}

void showList(TList *lst) {
    lst->workersNum ++;    
    pthread_mutex_lock(&lst->lock);
    
    if(lst->isDestroyed == 1){
        lst->workersNum--;
        pthread_mutex_unlock(&lst->lock);
        pthread_cond_signal(&lst->canDestroy);
        return;
    } 
    
    struct TNode *current = lst->head;
    printf("List: ");
    while (current) {
        printf("%p -> ", current->data);
        current = current->next;
    }
    printf("NULL\n");
    
    lst->workersNum--;
    pthread_mutex_unlock(&lst->lock);
}