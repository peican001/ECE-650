
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "my_malloc.h"

meta* freehead = NULL;
meta* allhead = NULL;

size_t allsize = 0;
size_t freesize = 0;
#define sizeM sizeof(meta)

Mlist freelist = { .head = NULL, .tail = NULL, .type = 0};
Mlist alllist = { .head = NULL, .tail = NULL, .type = 1};

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

void* newalloc(size_t size){
    size_t sizeneeded = size + sizeM;
    meta* newunit = sbrk(sizeneeded);

    if((void*)newunit == (void*)(-1)){
        return NULL;
    }

    allsize += sizeneeded;
    metaconstruct(newunit, size, 1);
    addunit(&alllist, newunit);
    
    return (char*)newunit + sizeM;
}

void coalesce(meta* unit){
    if(unit->used == 0){
        //right part
        if(unit->allnext != NULL && unit->allnext->used == 0 && (meta*)((char*)unit + sizeM + unit->size) == unit->allnext){
            meta* nextunit = unit->allnext;
            unit->size += sizeM + nextunit->size;
            removeunit(&freelist, nextunit);
            removeunit(&alllist, nextunit);
        }

        //left part
        if(unit->allprev != NULL && unit->allprev->used == 0 && (meta*)((char*)unit->allprev + sizeM + unit->allprev->size) == unit){
            meta* Prev = unit->allprev;
            Prev->size += sizeM + unit->size;
            removeunit(&freelist, unit);
            removeunit(&alllist, unit);
        }

    }
}

void freeunit(meta* unit){
    unit->used = 0;
    freesize += unit->size + sizeM;
    
    unit->freenext = NULL;
    unit->freeprev = NULL;
    addunit(&freelist, unit);
    coalesce(unit);

}

meta* divideunit(meta* unit, size_t size){
    unit->used = 1;
    if(size + sizeM <= unit->size){
        size_t oldsize = unit->size;
        unit->size = size;
        meta* newaddr = (meta*)((char*)unit + sizeM + size);
        size_t newsize = oldsize - size - sizeM;
        metaconstruct(newaddr, newsize, 0);

        //unit->used = 1;
        addunit(&freelist, newaddr);
        removeunit(&freelist, unit);

        meta* Next = unit->allnext;
        if(Next != NULL){
            Next->allprev = newaddr;
            newaddr->allnext = Next;
        }
        else{
            alllist.tail = newaddr;
        }
        newaddr->allprev = unit;
        unit->allnext = newaddr;
        
        freesize -= (sizeM + size);
    }
    else{
        freesize -= (sizeM + unit->size);
        removeunit(&freelist, unit);
    }
    return unit;
}

unsigned long get_data_segment_size(){
    return allsize;
}

unsigned long get_data_segment_free_space_size(){
    return freesize;
}

//malloc func

void* ff_malloc(size_t size){
    meta* curr = freelist.head;
    while(curr != NULL){
        if(curr->used == 0 && curr->size >= size){
        //if(curr->size >= size){
            return (char*)divideunit(curr, size) + sizeM;
        }
        curr = curr->freenext;
    }
    return newalloc(size);
}

void ff_free(void *ptr){
    if(ptr == NULL){
        return;
    }
    meta* unitFreed = (meta*)((char*)ptr - sizeM);
    if(unitFreed->used == 1){
        freeunit(unitFreed);
    }
}

void *bf_malloc(size_t size){
  meta* curr = freelist.head;
  meta* bestFit = NULL;
  size_t minSize = SIZE_MAX;

  while(curr != NULL){
    if(curr->used == 0){
        if(curr->size == size){
            return (char*)divideunit(curr, size) + sizeM;
        }
        else if(curr->size > size && curr->size < minSize){
            
            bestFit = curr;
            minSize = curr->size;
        }
    }
    curr = curr->freenext;
  }
  if(bestFit == NULL){
    return newalloc(size);
  }
  else{
    return (char*)divideunit(bestFit, size) + sizeM;
  }
}

void bf_free(void *ptr){
  ff_free(ptr);
}
