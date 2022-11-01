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

const size_t mem_size = 1 << 18; // 2^12

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
	int *iVar = (int*) malloc(sizeof(int));
	printf("%ld\n", (long) reinterpret_cast<std::uintptr_t>(iVar));
	std::vector<long> eviction_set;

	int iterations = 16;
	int pruning_iter = 5;
	for (int k = 0; k < iterations; k++)	{
		char* g_mem =  (char*)mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	    assert(g_mem != MAP_FAILED);
	    memset(g_mem, 0xff, mem_size);

	    std::vector<long> collision_set;
	    uint64_t *end = (uint64_t *) (g_mem + mem_size);
	    uint64_t *ptr;
	    int counter = 0;
	    long candidate_set[2 * (mem_size/64) + 1] = {};
	    for (ptr = (uint64_t *) g_mem; ptr < end; ptr=ptr+8)	{
	    	long got = (long) reinterpret_cast<std::uintptr_t>(ptr);
	    	candidate_set[counter] = got;
	    	counter++;
	    }
	    printf("size of set M = %d\n", counter);
	    candidate_set[counter] = (long) reinterpret_cast<std::uintptr_t>(iVar);
	    for (int i = 0; i < counter; i++)	{
	    	candidate_set[(counter + 1) + i] = candidate_set[i];
	    }
	    int arr_1[mem_size/64 + 1];
	   	callModuleFunc(candidate_set, (sizeof(candidate_set)/sizeof(candidate_set[0])), arr_1);
	   	int num_eviction = 0;
	    for (int i = 0 ; i < sizeof(arr_1)/sizeof(arr_1[0]); i++)	{
			if(arr_1[i] > 200)	{
	   			num_eviction++;
	   			if (candidate_set[i] != (long) reinterpret_cast<std::uintptr_t>(iVar))	{
	   				collision_set.push_back(candidate_set[i]);
	   			}
		    }
	    }
	    printf("No. of collisions = %d\n", num_eviction - 1);
	    // finding the self-evictions
	    printf(" [x]     Starting the pruning now ...........\n");
	    for (int j = 0; j < pruning_iter; j++)	{
		    long probing_set[2 * collision_set.size()];
		    for (int i = 0; i < 2 * collision_set.size(); i++)
		    	probing_set[i] = collision_set.at(i % collision_set.size());
		    int arr_2[collision_set.size()];
		   	callModuleFunc(probing_set, (sizeof(probing_set)/sizeof(probing_set[0])), arr_2);
		   	int self_eviction = 0;
		    for (int i = 0 ; i < sizeof(arr_2)/sizeof(arr_2[0]); i++)	{
				if(arr_2[i] > 200)	{
		   			self_eviction++;
	   				collision_set.at(i) = 0;
			    }
		    }
		    printf("no. of self evictions = %d\n", self_eviction);
		    collision_set.erase( std::remove (collision_set.begin(), collision_set.end(), 0), collision_set.end() );
    		printf("final size = %d\n", (int) collision_set.size());
		}
		for (int i = 0; i < collision_set.size(); ++i)	{
			eviction_set.push_back(collision_set.at(i));
		}
		munmap(g_mem, mem_size);
	}
	remove_duplicates(eviction_set);
	printf("size of eviction_set before self_eviction = %d\n", (int) eviction_set.size());
	while(true)	{
		long priming_set[2 * eviction_set.size()];
	    for (int i = 0; i < 2 * eviction_set.size(); i++)
	    	priming_set[i] = eviction_set.at(i % eviction_set.size());
	    int arr_3[eviction_set.size()];
	   	callModuleFunc(priming_set, (sizeof(priming_set)/sizeof(priming_set[0])), arr_3);
	   	int evs_in_ev = 0;
	    for (int i = 0 ; i < sizeof(arr_3)/sizeof(arr_3[0]); i++)	{
			if(arr_3[i] > 200)	{
	   			evs_in_ev++;
				eviction_set.at(i) = 0;
		    }
	    }
	    printf("no. of self evictions = %d\n", evs_in_ev );
	    eviction_set.erase( std::remove (eviction_set.begin(), eviction_set.end(), 0), eviction_set.end() );
	    if (evs_in_ev == 0)
			break;
	}
	int evset_size = (int) eviction_set.size();
	printf("final eviction set size = %d\n", evset_size);

	// // do the actual eviction now
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
    int arr_4[evset_size + 1];
    std::vector<int> timing[evset_size];
    std::default_random_engine generator;
  	std::normal_distribution<double> dist_miss(600,66.0);

  	for (int i = 0; i < 1000; i++)	{ 
	    callModuleFunc(ppp_set, (sizeof(ppp_set)/sizeof(ppp_set[0])), arr_4);    
	    for (int j=0 ; j < sizeof(arr_4)/sizeof(arr_4[0]); j++)	{
			if(arr_4[j] > 200)	{
	   			if (ppp_set[j] != (long) reinterpret_cast<std::uintptr_t>(iVar))	{
	   				timing[j].push_back((int) dist_miss(generator));
	   			}
		    }
	    }
	}

	FILE *fp = fopen("timing_new_algorithm_run_4.txt", "w");
	for (int i = 0; i < evset_size; i++)	{
		for (auto it = timing[i].begin(); it!= timing[i].end(); ++it)	{
			fprintf(fp, "%d ", *it);
		}
		fprintf(fp,"\n");
	}
	fclose(fp);
	return 0;
}