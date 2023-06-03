
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

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

void removeunit(Mlist* list, meta* unit);


void* newalloc(size_t size);


meta* divideunit(meta* unit, size_t size);

void coalesce(meta* unit);

void freeunit(meta* unit);


void* ff_malloc(size_t size);


void ff_free(void *ptr);


void *bf_malloc(size_t size);

void bf_free(void *ptr);


unsigned long get_data_segment_size();


unsigned long get_data_segment_free_space_size();
