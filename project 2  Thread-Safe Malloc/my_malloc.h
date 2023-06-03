#ifndef __MY_MALLOC_H__
#define __MY_MALLOC_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

//-------------------- pthread mechanisms --------------------//
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

//-------------------- function pointers --------------------//

typedef void*(*sbrkFp)(intptr_t);

//-------------------- free list data structure --------------------//

typedef struct _Metadata {
  size_t size;
  int used;
  struct _Metadata* freeprev;
  struct _Metadata* freenext;
  struct _Metadata* allprev;
  struct _Metadata* allnext;

} meta;

typedef struct _Mlist {
  meta* head;
  meta* tail;
  int type;//0: free, 1: all
} Mlist;

int checkheadandtail();

int checkfree();
int checkall();
void metaconstruct(meta* unit, size_t size, int used);


//void appendFree(Mlist* list, meta* unit);

//void addunit(Mlist* blklist, meta* unit, meta* Prev);

void addunit(Mlist* list, meta* unit);
void tsaddunit(meta* head, meta* tail, int type, meta* unit);

void removeunit(Mlist* list, meta* unit);
void tsremoveunit(meta* head, meta* tail, int type, meta* unit);

void* sbrk_tls(size_t datasize);
//void* newalloc(size_t size, sbrkFp fp, Mlist* Alllist);

void* newalloc(size_t size, Mlist* Alllist);
void* tsnewalloc(size_t size, meta* allhead, meta* alltail);


meta* divideunit(meta* unit, size_t size, Mlist* Freelist, Mlist* Alllist);
meta* tsdivideunit(meta* unit, size_t size, meta* freehead, meta* freetail, meta* allhead, meta* alltail);

void coalesce(meta* unit, Mlist* Freelist, Mlist* Alllist);
void tscoalesce(meta* unit, meta* freehead, meta* freetail, meta* allhead, meta* alltail);

void freeunit(meta* unit, Mlist* Freelist, Mlist* Alllist);
void tsfreeunit(meta* unit, meta* freehead, meta* freetail, meta* allhead, meta* alltail);


void* ff_malloc(size_t size, sbrkFp fp, Mlist* Freelist, Mlist* Alllist);


void ff_free(void *ptr, Mlist* Freelist, Mlist* Alllist);


//void *bf_malloc(size_t size, sbrkFp fp, Mlist* Freelist, Mlist* Alllist);
void *bf_malloc(size_t size, Mlist* Freelist, Mlist* Alllist);
void *tsbf_malloc(size_t size, meta* freehead, meta* freetail, meta* allhead, meta* alltail);

void bf_free(void *ptr, Mlist* Freelist, Mlist* Alllist);
void tsbf_free(void *ptr, meta* freehead, meta* freetail, meta* allhead, meta* alltail);


//unsigned long get_data_segment_size();


//unsigned long get_data_segment_free_space_size();

//thread safe part


void* sbrk_tls(size_t datasize);

void *ts_malloc_lock(size_t size);


void ts_free_lock(void *ptr);


void *ts_malloc_nolock(size_t size);


void ts_free_nolock(void *ptr);


#endif