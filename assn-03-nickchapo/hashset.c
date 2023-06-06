#include "hashset.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static const int defaultSize = 4;
static const int kNotFound = -1;

void HashSetNew(hashset *h, int elemSize, int numBuckets,
		HashSetHashFunction hashfn, HashSetCompareFunction comparefn, HashSetFreeFunction freefn)
{
	assert(elemSize>0 && numBuckets>0);
	assert(hashfn != NULL && comparefn != NULL);
	h->elemSize		= elemSize;
	h->numBuckets	= numBuckets;
	h->hashfn		= hashfn;
	h->comparefn	= comparefn;
	h->freefn		= freefn;
	h->logLen		= 0;
	h->elems		= (vector*)malloc(numBuckets * sizeof(vector));
	
	for(int i=0; i < numBuckets; i++){
		VectorNew(h->elems + i , elemSize, freefn, defaultSize);
	}
}

void HashSetDispose(hashset *h)
{
	for(int i=0;i<h->numBuckets;i++){
		VectorDispose(h->elems + i);
	}
	free(h->elems);
}

int HashSetCount(const hashset *h)
{ 
	return h->logLen; 
}

void HashSetMap(hashset *h, HashSetMapFunction mapfn, void *auxData)
{
	assert(mapfn != NULL);

	for(int i=0; i < h->numBuckets; i++){
		VectorMap(h->elems + i, mapfn, auxData);
	}
}

void HashSetEnter(hashset *h, const void *elemAddr)
{
	assert(elemAddr != NULL);
	int bucket = h->hashfn(elemAddr, h->numBuckets);
	assert(bucket >= 0 && bucket < h->numBuckets);

	int index = VectorSearch(h->elems + bucket, elemAddr, h->comparefn, 0, true);

	if(index == kNotFound){
		VectorAppend(h->elems + bucket, elemAddr);
		VectorSort(h->elems + bucket, h->comparefn);
		h->logLen++;
	}else{
		VectorReplace(h->elems + bucket, elemAddr, index);
	}
}

void *HashSetLookup(const hashset *h, const void *elemAddr)
{ 
	assert(elemAddr != NULL);
	int bucket = h->hashfn(elemAddr, h->numBuckets);
	assert(bucket >= 0 && bucket < h->numBuckets);

	//VectorSort(h->elems + bucket, h->comparefn);
	int index = VectorSearch(h->elems + bucket, elemAddr, h->comparefn, 0, true);
	
	if(index != kNotFound){
		return VectorNth(h->elems + bucket, index);
	}
	return NULL; 
}