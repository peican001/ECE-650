
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "my_malloc.h"
#include "assert.h"


__thread meta* tsFreeHead = NULL;
__thread meta* tsFreeTail = NULL;

__thread meta* tsAllHead = NULL;
__thread meta* tsAllTail = NULL;

//size_t allsize = 0;
//size_t freesize = 0;
#define sizeM sizeof(meta)

Mlist freelist = { .head = NULL, .tail = NULL, .type = 0};
Mlist alllist = { .head = NULL, .tail = NULL, .type = 1};

//__thread Mlist tsfreelist = { .head = NULL, .tail = NULL, .type = 0};
//__thread Mlist tsalllist = { .head = NULL, .tail = NULL, .type = 1};



//helper func

int checkheadandtail(){
    if(freelist.head == NULL && freelist.tail == NULL && alllist.head == NULL && alllist.tail == NULL){
        return 0;
    }
    else if(freelist.head->freeprev == NULL && freelist.tail->freenext == NULL && alllist.head->allprev == NULL && alllist.tail->allnext == NULL){
        meta* freecurr = freelist.head;
        while(freecurr->freenext != NULL){
            freecurr = freecurr->freenext;
        }
        if (freecurr != freelist.tail){
            return 3;
        }
        meta* allcurr = alllist.head;
        while(allcurr->allnext != NULL){
            allcurr = allcurr->allnext;
        }
        if (allcurr != alllist.tail){
            return 4;
        }
        return 1;
    }
    else{
        return 2;
    }
}

int checkfree(){
    meta* curr = freelist.head;
    while(curr != NULL){
        if(curr->used != 0){
            return 0;
        }
        curr = curr->freenext;
    }
    return 1;

}

int checkall(){
    meta* curr = alllist.head;
    while(curr != NULL){
        if(curr->allnext != NULL){
            if(curr->used == curr->allnext->used && curr->used == 0){
                return 0;
            }
            meta* next = (meta*)((char*)curr + sizeM + curr->size);
            if(curr->allnext != next){
                return 1;
            }
        }
        curr = curr->allnext;
    }
    return 2;
}

void metaconstruct(meta* unit, size_t size, int used){
    unit->size = size;
    unit->used = used;
    unit->freeprev = NULL;
    unit->freenext = NULL;
    unit->allprev = NULL;
    unit->allnext = NULL;
}

void addunit(Mlist* list, meta* unit){
    if(list->head == NULL && list->tail == NULL){
        list->head = unit;
        //list->tail = unit;
    }
    else if(list->type == 0){
        list->tail->freenext = unit;
        unit->freeprev = list->tail;
        unit->freenext = NULL;
        //list->tail = unit;
    }
    else{
        list->tail->allnext = unit;
        unit->allprev = list->tail;
        unit->allnext = NULL;
        //list->tail = unit;
    }
    list->tail = unit;

}

void tsaddunit(meta* head, meta* tail, int type, meta* unit){
    if(head == NULL && tail == NULL){
        head = unit;
        //list->tail = unit;
    }
    else if(type == 0){
        tail->freenext = unit;
        unit->freeprev = tail;
        unit->freenext = NULL;
        //list->tail = unit;
    }
    else{
        tail->allnext = unit;
        unit->allprev = tail;
        unit->allnext = NULL;
        //list->tail = unit;
    }
    tail = unit;

}

void removeunit(Mlist* list, meta* unit){
    
    if(list->head == list->tail && list->head == unit){
        list->head = NULL;
        list->tail = NULL;
    }
    else if(list->type == 0){
        
        if(unit == list->head){
            //list->head->freeprev = NULL;
            list->head = unit->freenext;
            list->head->freeprev = NULL;
        }
        else if(unit == list->tail){
            //list->tail->freenext = NULL;
            list->tail = unit->freeprev;
            list->tail->freenext = NULL;
        }
        else{
            unit->freeprev->freenext = unit->freenext;
            unit->freenext->freeprev = unit->freeprev;
        }

        unit->freeprev = NULL;
        unit->freenext = NULL;
    }
    else if(list->type == 1){
        
        if(unit == list->head){
            //list->head->allprev = NULL;
            list->head = unit->allnext;
            list->head->allprev = NULL;
        }
        else if(unit == list->tail){
            //list->tail->allnext = NULL;
            list->tail = unit->allprev;
            list->tail->allnext = NULL;
        }
        else{
            unit->allprev->allnext = unit->allnext;
            unit->allnext->allprev = unit->allprev;
        }

        unit->allprev = NULL;
        unit->allnext = NULL;
    }
    unit->used = 1;
}

void tsremoveunit(meta* head, meta* tail, int type, meta* unit){
    
    if(head == tail && head == unit){
        head = NULL;
        tail = NULL;
    }
    else if(type == 0){
        
        if(unit == head){
            //list->head->freeprev = NULL;
            head = unit->freenext;
            head->freeprev = NULL;
        }
        else if(unit == tail){
            //list->tail->freenext = NULL;
            tail = unit->freeprev;
            tail->freenext = NULL;
        }
        else{
            unit->freeprev->freenext = unit->freenext;
            unit->freenext->freeprev = unit->freeprev;
        }

        unit->freeprev = NULL;
        unit->freenext = NULL;
    }
    else if(type == 1){
        
        if(unit == head){
            //list->head->allprev = NULL;
            head = unit->allnext;
            head->allprev = NULL;
        }
        else if(unit == tail){
            //list->tail->allnext = NULL;
            tail = unit->allprev;
            tail->allnext = NULL;
        }
        else{
            unit->allprev->allnext = unit->allnext;
            unit->allnext->allprev = unit->allprev;
        }

        unit->allprev = NULL;
        unit->allnext = NULL;
    }
    unit->used = 1;
}

void* sbrk_tls(size_t datasize){
  assert(pthread_mutex_lock(&lock) == 0);
  void* ptr = sbrk(datasize);
  assert(pthread_mutex_unlock(&lock) == 0);
  return ptr;
}

void* newalloc(size_t size, Mlist* Alllist){
    size_t sizeneeded = size + sizeM;
    meta* newunit = sbrk(sizeneeded);

    if((void*)newunit == (void*)(-1)){
        return NULL;
    }

    //allsize += sizeneeded;
    metaconstruct(newunit, size, 1);
    addunit(Alllist, newunit);
    
    return (char*)newunit + sizeM;
}

void* tsnewalloc(size_t size, meta* allhead, meta* alltail){
    size_t sizeneeded = size + sizeM;
    meta* newunit = sbrk_tls(sizeneeded);

    if((void*)newunit == (void*)(-1)){
        return NULL;
    }

    //allsize += sizeneeded;
    metaconstruct(newunit, size, 1);
    tsaddunit(allhead, alltail, 1, newunit);
    
    return (char*)newunit + sizeM;
}

void coalesce(meta* unit, Mlist* Freelist, Mlist* Alllist){
    if(unit->used == 0){
        //right part
        if(unit->allnext != NULL && unit->allnext->used == 0 && (meta*)((char*)unit + sizeM + unit->size) == unit->allnext){
            meta* nextunit = unit->allnext;
            unit->size += sizeM + nextunit->size;
            removeunit(Freelist, nextunit);
            removeunit(Alllist, nextunit);
        }

        //left part
        if(unit->allprev != NULL && unit->allprev->used == 0 && (meta*)((char*)unit->allprev + sizeM + unit->allprev->size) == unit){
            meta* Prev = unit->allprev;
            Prev->size += sizeM + unit->size;
            removeunit(Freelist, unit);
            removeunit(Alllist, unit);
        }

    }
}

void tscoalesce(meta* unit, meta* freehead, meta* freetail, meta* allhead, meta* alltail){
    if(unit->used == 0){
        //right part
        if(unit->allnext != NULL && unit->allnext->used == 0 && (meta*)((char*)unit + sizeM + unit->size) == unit->allnext){
            meta* nextunit = unit->allnext;
            unit->size += sizeM + nextunit->size;
            tsremoveunit(freehead, freetail, 0, nextunit);
            tsremoveunit(allhead, alltail, 1, nextunit);
        }

        //left part
        if(unit->allprev != NULL && unit->allprev->used == 0 && (meta*)((char*)unit->allprev + sizeM + unit->allprev->size) == unit){
            meta* Prev = unit->allprev;
            Prev->size += sizeM + unit->size;
            tsremoveunit(freehead, freetail, 0, unit);
            tsremoveunit(allhead, alltail, 1, unit);
        }

    }
}

void freeunit(meta* unit, Mlist* Freelist, Mlist* Alllist){
    unit->used = 0;
    //freesize += unit->size + sizeM;
    
    unit->freenext = NULL;
    unit->freeprev = NULL;
    addunit(Freelist, unit);
    coalesce(unit, Freelist, Alllist);

}

void tsfreeunit(meta* unit, meta* freehead, meta* freetail, meta* allhead, meta* alltail){
    unit->used = 0;
    //freesize += unit->size + sizeM;
    
    unit->freenext = NULL;
    unit->freeprev = NULL;
    tsaddunit(freehead, freetail, 0, unit);
    tscoalesce(unit, freehead, freetail, allhead, alltail);

}

meta* divideunit(meta* unit, size_t size, Mlist* Freelist, Mlist* Alllist){
    unit->used = 1;
    if(size + sizeM < unit->size){
        size_t oldsize = unit->size;
        unit->size = size;
        meta* newaddr = (meta*)((char*)unit + sizeM + size);
        size_t newsize = oldsize - size - sizeM;
        metaconstruct(newaddr, newsize, 0);

        //unit->used = 1;
        addunit(Freelist, newaddr);
        removeunit(Freelist, unit);

        meta* Next = unit->allnext;
        if(Next != NULL){
            Next->allprev = newaddr;
            newaddr->allnext = Next;
        }
        else{
            Alllist->tail = newaddr;
        }
        newaddr->allprev = unit;
        unit->allnext = newaddr;
        
        //freesize -= (sizeM + size);
    }
    else{
        //freesize -= (sizeM + unit->size);
        removeunit(Freelist, unit);
    }
    return unit;
}

meta* tsdivideunit(meta* unit, size_t size, meta* freehead, meta* freetail, meta* allhead, meta* alltail){
    unit->used = 1;
    if(size + sizeM < unit->size){
        size_t oldsize = unit->size;
        unit->size = size;
        meta* newaddr = (meta*)((char*)unit + sizeM + size);
        size_t newsize = oldsize - size - sizeM;
        metaconstruct(newaddr, newsize, 0);

        //unit->used = 1;
        tsaddunit(freehead, freetail, 0, newaddr);
        tsremoveunit(freehead, freetail, 0, unit);

        meta* Next = unit->allnext;
        if(Next != NULL){
            Next->allprev = newaddr;
            newaddr->allnext = Next;
        }
        else{
            alltail = newaddr;
        }
        newaddr->allprev = unit;
        unit->allnext = newaddr;
        
        //freesize -= (sizeM + size);
    }
    else{
        //freesize -= (sizeM + unit->size);
        tsremoveunit(freehead, freetail, 0, unit);
    }
    return unit;
}

/*
unsigned long get_data_segment_size(){
    return allsize;
}

unsigned long get_data_segment_free_space_size(){
    return freesize;
}
*/

//malloc func

void* ff_malloc(size_t size, sbrkFp fp, Mlist* Freelist, Mlist* Alllist){
    meta* curr = Freelist->head;
    while(curr != NULL){
        if(curr->used == 0 && curr->size >= size){
        //if(curr->size >= size){
            return (char*)divideunit(curr, size, Freelist, Alllist) + sizeM;
        }
        curr = curr->freenext;
    }
    return newalloc(size, Alllist);
}

void ff_free(void *ptr, Mlist* Freelist, Mlist* Alllist){
    if(ptr == NULL){
        return;
    }
    meta* unitFreed = (meta*)((char*)ptr - sizeM);
    if(unitFreed->used == 1){
        freeunit(unitFreed, Freelist, Alllist);
    }
}

void *bf_malloc(size_t size, Mlist* Freelist, Mlist* Alllist){
  meta* curr = Freelist->head;
  meta* bestFit = NULL;
  size_t minSize = SIZE_MAX;

  while(curr != NULL){
    if(curr->used == 0){
        if(curr->size == size){
            return (char*)divideunit(curr, size, Freelist, Alllist) + sizeM;
        }
        else if(curr->size > size && curr->size < minSize){
            
            bestFit = curr;
            minSize = curr->size;
        }
    }
    curr = curr->freenext;
  }
  if(bestFit == NULL){
    return newalloc(size, Alllist);
  }
  else{
    return (char*)divideunit(bestFit, size, Freelist, Alllist) + sizeM;
  }
}

void *tsbf_malloc(size_t size, meta* freehead, meta* freetail, meta* allhead, meta* alltail){
  meta* curr = freehead;
  meta* bestFit = NULL;
  size_t minSize = SIZE_MAX;

  while(curr != NULL){
    if(curr->used == 0){
        if(curr->size == size){
            return (char*)tsdivideunit(curr, size, freehead, freetail, allhead, alltail) + sizeM;
        }
        else if(curr->size > size && curr->size < minSize){
            
            bestFit = curr;
            minSize = curr->size;
        }
    }
    curr = curr->freenext;
  }
  if(bestFit == NULL){
    return tsnewalloc(size, allhead, alltail);
  }
  else{
    return (char*)tsdivideunit(bestFit, size, freehead, freetail,allhead,alltail) + sizeM;
  }
}

void bf_free(void *ptr, Mlist* Freelist, Mlist* Alllist){
  ff_free(ptr, Freelist, Alllist);
}

void tsbf_free(void *ptr, meta* freehead, meta* freetail, meta* allhead, meta* alltail){
    if(ptr == NULL){
        return;
    }
    meta* unitFreed = (meta*)((char*)ptr - sizeM);
    if(unitFreed->used == 1){
        tsfreeunit(unitFreed, freehead, freetail, allhead, alltail);
    }
}

//-------------------- threaded malloc/free API --------------------//

void *ts_malloc_lock(size_t size){
  assert(pthread_mutex_lock(&lock) == 0);
  void* ptr = bf_malloc(size, &freelist, &alllist);
  assert(pthread_mutex_unlock(&lock) == 0);
  return ptr;
}

void ts_free_lock(void *ptr){
  assert(pthread_mutex_lock(&lock) == 0);
  bf_free(ptr, &freelist, &alllist);
  assert(pthread_mutex_unlock(&lock) == 0);
}

void *ts_malloc_nolock(size_t size){
  void* ptr = tsbf_malloc(size, tsFreeHead, tsFreeTail, tsAllHead, tsAllTail);
  return ptr;
}

void ts_free_nolock(void *ptr){
  tsbf_free(ptr, tsFreeHead, tsFreeTail, tsAllHead, tsAllTail);
}
