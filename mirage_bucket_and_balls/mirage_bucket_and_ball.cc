#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "mtrand.h"

int EXTRA_BUCKET_CAPACITY = 0;
int NUM_TRIALS = 1 * 1000 * 1000;

unsigned int seed = 1;

#define BASE_WAYS_PER_SKEW        (8)

#define NUM_SKEWS                 (2)

//16MB LLC
#define CACHE_SZ_BYTES            (16*1024*1024) 
#define LINE_SZ_BYTES             (64)
#define NUM_BUCKETS               ((CACHE_SZ_BYTES/LINE_SZ_BYTES)/(BASE_WAYS_PER_SKEW))
#define NUM_BUCKETS_PER_SKEW      (NUM_BUCKETS/NUM_SKEWS)

//Bucket Capacities
#define BALLS_PER_BUCKET      (BASE_WAYS_PER_SKEW)
#define MAX_FILL              (16)

#define MAXN					  NUM_BUCKETS_PER_SKEW 


int SPILL_THRESHOLD = BALLS_PER_BUCKET + EXTRA_BUCKET_CAPACITY;

typedef unsigned int uns;
typedef unsigned long long uns64;
typedef double dbl;

uns64 buckets[NUM_BUCKETS];

MTRand *mtrand=new MTRand();


int insert_ball(uns64 ballID)	{
	uns64 index1 = mtrand->randInt(NUM_BUCKETS_PER_SKEW  - 1);
	uns64 index2 = NUM_BUCKETS_PER_SKEW + mtrand->randInt(NUM_BUCKETS_PER_SKEW - 1);

	uns64 index;
	uns retval;

	if (buckets[index2] < buckets[index1])	
		index = index2;
	else if (buckets[index1] < buckets[index2])
		index = index1;
	else	{
		if (mtrand->randInt(1) == 0)	
			index = index1;
		else
			index = index2;
	}

	retval = buckets[index];
	if (retval >= SPILL_THRESHOLD)	{
	 	return -1;
	}
	else	{
		buckets[index]++;
		return 0;
	}

	return 0;

}

void init_buckets()	{
	uns64 ii;
	assert(NUM_SKEWS * NUM_BUCKETS_PER_SKEW == NUM_BUCKETS);

	for (ii = 0; ii < NUM_BUCKETS; ii++)
		buckets[ii] = 0;
}


int main(int argc, char* argv[])	{

	init_buckets();

	assert(argc == 2);
	EXTRA_BUCKET_CAPACITY = atoi(argv[1]);
	SPILL_THRESHOLD = BASE_WAYS_PER_SKEW + EXTRA_BUCKET_CAPACITY;

	FILE *fp = fopen("mirage_bucket_ball_data.txt", "a");

	printf("%d, %d\n", BASE_WAYS_PER_SKEW, EXTRA_BUCKET_CAPACITY);

	printf("Cache configurations: %d MB, %d skews, %d ways (%d ways/skew)\n", CACHE_SZ_BYTES/1024/1024,NUM_SKEWS,NUM_SKEWS*BASE_WAYS_PER_SKEW,BASE_WAYS_PER_SKEW);
	printf("Number of buckets: %d\n", NUM_BUCKETS);

	for (uns64 i = 0; i < NUM_TRIALS; i++)	{
		if (insert_ball(i) != 0)	{
			printf("Found collision after %lld trials\n", i);
			fprintf(fp, "%lld\n", i);
			break;
		}
	}

	return 0;
}