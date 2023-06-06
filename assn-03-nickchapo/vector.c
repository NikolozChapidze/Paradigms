#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <search.h>

static const int defaultSize = 4;

#define getPointer(v,index) ((char*)v->elems + (index * v->elemSize))

void checkCapacity(vector *v){
    if(v->logLen == v->allocLen){
        v->allocLen = v->allocLen * 2;
        v->elems    = realloc(v->elems, v->allocLen * v->elemSize);
    }
    assert(v->elems != NULL);
}

void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation)
{
    assert(elemSize > 0);
    v->elemSize = elemSize;
    v->freefn   = freeFn;
    v->logLen   = 0;
    v->allocLen = (initialAllocation == 0) ? defaultSize : initialAllocation;
    assert(v->allocLen >= 0);
    v->elems    = malloc(elemSize * v->allocLen);
    assert(v->elems != NULL);
}

void VectorDispose(vector *v)
{
    if(v->freefn != NULL){
        for(int i=0; i < v->logLen; i++){
            v->freefn(VectorNth(v,i));
        }
    }
    free(v->elems);
}

int VectorLength(const vector *v)
{ 
    return v->logLen; 
}

void *VectorNth(const vector *v, int position)
{ 
    assert(position >= 0 && position < v->logLen);
    void* result = getPointer(v, position);
    //(char*)v->elems + (position * v->elemSize);
    return result; 
}

void VectorReplace(vector *v, const void *elemAddr, int position)
{
    assert(position >= 0 && position < v->logLen);
    
    void* posAdr = getPointer(v,position);
    //VectorNth(v,position);
    if(v->freefn != NULL){
        v->freefn(posAdr);
    }
    memcpy(posAdr, elemAddr, v->elemSize);
}

void VectorInsert(vector *v, const void *elemAddr, int position)
{
    assert(position >= 0 && position <= v->logLen);

    checkCapacity(v);
    void* nthNextElem = getPointer(v, (position+1));
    //(char*)v->elems + ((position+1) * v->elemSize);
    void* nthElem     = getPointer(v, position);
    //(char*)v->elems + (position * v->elemSize);
    if(position < v->logLen){
        memmove(nthNextElem,nthElem, v->elemSize * (v->logLen - position));
    }
    memcpy(nthElem, elemAddr, v->elemSize);
    v->logLen++;
}

void VectorAppend(vector *v, const void *elemAddr)
{
    checkCapacity(v);
    memcpy(getPointer(v, (v->logLen)), elemAddr, v->elemSize);
    //(char*)v->elems + (v->logLen * v->elemSize)
    v->logLen++;
}

void VectorDelete(vector *v, int position)
{
    assert(position >= 0 && position < v->logLen);
    if(position != v->logLen){
        if(v->freefn != NULL){        v->freefn(getPointer(v,position));}
        memcpy(getPointer(v, position), getPointer(v, (position+1)), v->elemSize * (v->logLen-position-1));
        //(char*)v->elems + (position * v->elemSize)
        //(char*)v->elems + ((position+1) * v->elemSize)
    }
    v->logLen--;
}

void VectorSort(vector *v, VectorCompareFunction compare)
{
    assert(compare != NULL);
    qsort(v->elems, v->logLen, v->elemSize, compare);
}

void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData)
{
    assert(mapFn != NULL);
    //printf("%d", v->logLen);
    for(int i=0; i < v->logLen; i++){
        mapFn((char*)v->elems + (v->elemSize * i), auxData);
    }
}

static const int kNotFound = -1;
int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted)
{ 
    assert(startIndex>=0 && startIndex<=v->logLen);
    assert(key != NULL && searchFn != NULL);
    void* result;

    size_t sizeOfFind = v->logLen-startIndex;
    if(isSorted){
        result = bsearch(key, getPointer(v, startIndex), sizeOfFind, v->elemSize, searchFn);
        //(char*)v->elems + (v->elemSize * startIndex)
    }else{
        result = lfind(key, getPointer(v, startIndex), &sizeOfFind, v->elemSize, searchFn);
        //(char*)v->elems + (v->elemSize * startIndex)
    }
    if(result != NULL){
        return ((char*)result - (char*)v->elems)/v->elemSize;
    }
    return kNotFound; 
} 
