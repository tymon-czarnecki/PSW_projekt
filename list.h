#ifndef LIST_H
#define LIST_H

#include <pthread.h>

struct TList {
    struct TNode *head;
    struct TNode *tail;
    int size;
    int maxSize;
    int isDestroyed;
    int workersNum;
    pthread_mutex_t lock;
    pthread_cond_t canAdd;
    pthread_cond_t canRemove;
    pthread_cond_t canDestroy;
};

struct TNode {
    void *data;
    struct TNode *next;
};

TList *createList(int s);
void destroyList(TList *lst);
void putItem(TList *lst, void *itm);
void *getItem(TList *lst);
void *popItem(TList *lst);
int removeItem(TList *lst, void *itm);
int getCount(TList *lst);
void setMaxSize(TList *lst, int s);
void appendItems(TList *lst, TList *lst2);
void showList(TList *lst);

#endif // LIST_H
