#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <cstdint>
#include <vector>
#include <iostream>
#include <algorithm>
#include <random>

using namespace std;
 
const size_t mem_size = 1 << 18;

void initPython()   {
    Py_Initialize();
    PyObject *sysmodule = PyImport_ImportModule("sys");
    PyObject *syspath = PyObject_GetAttrString(sysmodule, "path");
    PyList_Append(syspath, PyUnicode_FromString("."));
}

void callModuleFunc(long array[], size_t size, int* temp) {
    PyObject *pmodule = PyImport_ImportModule("main");
    assert(pmodule != NULL);
    PyObject *pfunc = PyObject_GetAttrString(pmodule, (char *)"main");
    assert(pfunc != NULL);
    PyObject *plist = PyList_New(size);
    for (size_t i = 0; i != size; ++i) {
        PyList_SET_ITEM(plist, i, PyLong_FromLong(array[i]));
    }
    PyObject *arglist = Py_BuildValue("(O)", plist);
    assert(arglist != NULL);
    PyObject *result = PyObject_CallObject(pfunc, arglist);
    assert(result != NULL);
    assert(PyList_Check(result) != 0);
    int count = (int) PyList_Size(result);

    for (int i = 0 ; i < count ; i++ )	{
        PyObject *ptemp = PyList_GetItem(result,i);
        *(temp + i) = (int) PyLong_AsLong(ptemp); 
    }    

    Py_DECREF(pmodule);
    Py_DECREF(pfunc);
    Py_DECREF(plist);
    Py_DECREF(result);
}

void remove_duplicates(std::vector<long> &v)
{
    auto end = v.end();
    for (auto it = v.begin(); it != end; ++it)	{
        end = std::remove(it + 1, end, *it);
    }
    v.erase(end, v.end());
}

int main(int argc, char* argv[])	{
	initPython();
	int *iVar = (int*)malloc(sizeof(int));
	printf("%ld\n", (long) reinterpret_cast<std::uintptr_t>(iVar));
	std::vector<long> eviction_set;

	int attacker_accesses = 10;
	for (int k = 0; k < attacker_accesses; k++)	{
		char* g_mem =  (char*)mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	    assert(g_mem != MAP_FAILED);
	    memset(g_mem, 0xff, mem_size);


	    int pruned_iter = 20;
	    std::vector<long> probing_set;
	    uint64_t *end = (uint64_t *) (g_mem + mem_size);
	    uint64_t *ptr;
	    int counter = 0;
	    int pruned_size = 0;
	    for (int iter = 0; iter < pruned_iter; iter++)	{
	    	if (iter == 0)	{
	    		long candidate_set[2 * (mem_size/64)] = {};
	    		for (ptr = (uint64_t *) g_mem; ptr < end; ptr=ptr+8) {
	    			long got = (long) reinterpret_cast<std::uintptr_t>(ptr);
	    			candidate_set[counter] = got;
	    			probing_set.push_back(got);
	    			counter++;
				}
				printf("Size of k = %d\n", counter);
				for (int j = 0; j < counter; j++)
					candidate_set[counter + j] = candidate_set[j];

				int arr[mem_size/64];
	    		callModuleFunc(candidate_set, (sizeof(candidate_set)/sizeof(candidate_set[0])), arr);
	    		int self_eviction = 0;
	    		for (int i=0 ; i < sizeof(arr)/sizeof(arr[0]); i++)	{
		    		if(arr[i] > 200)	{
			    		probing_set.at(i) = 0;
		    			self_eviction++;
		    		}
	    		}
	    		printf("no. of evictions = %d\n", self_eviction);
	    		probing_set.erase( std::remove (probing_set.begin(), probing_set.end(), 0), probing_set.end() );
	    		pruned_size = (int) probing_set.size();
	    		printf("final size = %d\n", pruned_size);
	    	}
	    	else	{
	    		pruned_size = (int) probing_set.size();
	    		long candidate_set[2 * pruned_size] = {};
	    		for (int i = 0; i < pruned_size; i++)	{
	    			candidate_set[i] = probing_set.at(i);
	    		}
	    		for (int j = 0; j < pruned_size; j++)
					candidate_set[pruned_size + j] = candidate_set[j];

	    		int arr[pruned_size];
	    		callModuleFunc(candidate_set, (sizeof(candidate_set)/sizeof(candidate_set[0])), arr);
	    		int self_eviction = 0;
	    		for (int i=0 ; i < sizeof(arr)/sizeof(arr[0]); i++)	{
		    		if(arr[i] > 200)	{
			    		probing_set.at(i) = 0;
		    			self_eviction++;
		    		}
	    		}
	    		printf("no. of evictions = %d\n", self_eviction);
	    		probing_set.erase( std::remove (probing_set.begin(), probing_set.end(), 0), probing_set.end() );
	    		pruned_size = (int) probing_set.size();
	    		printf("final size = %d\n", pruned_size);
	    		if (self_eviction <= 3)
	    			break;
	    	}
	    }	// end of pruning phase
	    //--------------------------------------------------------------------------
	    // finding the elements of eviction set
	    long priming_set[2 * pruned_size + 1];
	    for (int i = 0; i < (2 * pruned_size) + 1; ++i)
	    {
	    	if (i == pruned_size)
	    		priming_set[i] = (long) reinterpret_cast<std::uintptr_t>(iVar);
	    	else if	(i < pruned_size)
		    	priming_set[i] = probing_set.at(i % pruned_size);
	    	else
	    		priming_set[i] = probing_set.at((i - 1) % pruned_size);
	    }
	    int arr[pruned_size + 1];
	    callModuleFunc(priming_set, (sizeof(priming_set)/sizeof(priming_set[0])), arr);
	    int num_eviction = 0;
	    for (int i=0 ; i < sizeof(arr)/sizeof(arr[0]); i++)	{
			if(arr[i] > 200)	{
	   			num_eviction++;
	   			if (priming_set[i] != (long) reinterpret_cast<std::uintptr_t>(iVar))	{
	   				eviction_set.push_back(priming_set[i]);
	   			}
		    }
	    }
	    printf("evicted by victim = %d\n", num_eviction - 1);
	    munmap(g_mem, mem_size);
	    printf("------End of %d-th iteration----\n", k+1);
	} //eviction set complete
	printf("eviction set size = %d\n", (int) eviction_set.size());
	//----------------------------------------------------------------------------
	remove_duplicates(eviction_set);
    int evset_size = (int) eviction_set.size();
    printf("eviction set size = %d\n", evset_size);

    // remove self evicting addresses inside eviction set
    for (int i = 0; i < 10; i++)	{
	    while(true)	{
		    long final_set[2 * evset_size];
		    for (int j = 0; j < 2 * evset_size; j++)	{
		    	final_set[j] = eviction_set.at(j % evset_size);
		    }
		    int arr[evset_size];
			callModuleFunc(final_set, (sizeof(final_set)/sizeof(final_set[0])), arr);
			int evs_in_ev = 0;
			for (int i=0 ; i < sizeof(arr)/sizeof(arr[0]); i++)	{
	    		if(arr[i] > 200)	{
		    		eviction_set.at(i) = 0;
	    			evs_in_ev++;
	    		}
	    		printf("%ld\n", eviction_set.at(i));
			}
			printf("no. of evictions = %d\n", evs_in_ev);
			eviction_set.erase( std::remove (eviction_set.begin(), eviction_set.end(), 0), eviction_set.end() );
			evset_size = (int) eviction_set.size();
			printf("final size = %d\n", evset_size);
			if (evs_in_ev == 0)
				break;
		}
	}

    printf("eviction set size = %d\n", evset_size);


    // do the actual eviction now
	long ppp_set[(2 * evset_size) + 1];
	for (int i = 0; i < (2 * evset_size) + 1; ++i)
    {
    	if (i == evset_size)
    		ppp_set[i] = (long) reinterpret_cast<std::uintptr_t>(iVar);
    	else if	(i < evset_size)
	    	ppp_set[i] = eviction_set.at(i % evset_size);
    	else
    		ppp_set[i] = eviction_set.at((i - 1) % evset_size);
    }
    int arr[evset_size + 1];
    std::vector<int> timing[evset_size];
    std::default_random_engine generator;
  	std::normal_distribution<double> dist_miss(600,66.0);

  	for (int i = 0; i < 1000; i++)	{ 
	    callModuleFunc(ppp_set, (sizeof(ppp_set)/sizeof(ppp_set[0])), arr);    

	    for (int j=0 ; j < sizeof(arr)/sizeof(arr[0]); j++)	{
			if(arr[j] > 200)	{
	   			if (ppp_set[j] != (long) reinterpret_cast<std::uintptr_t>(iVar))	{
	   				timing[j].push_back((int) dist_miss(generator));
	   			}
		    }
	    }
	}

	FILE *fp = fopen("timing_normcache_onepart.txt", "w");
	for (int i = 0; i < evset_size; i++)	{
		for (auto it = timing[i].begin(); it!= timing[i].end(); ++it)	{
			fprintf(fp, "%d ", *it);
		}
		fprintf(fp,"\n");
	}
	fclose(fp);
    return 0;
}