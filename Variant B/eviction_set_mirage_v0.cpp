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
 
const size_t mem_size = 10000 * 64;

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


int main(int argc, char *argv[])	{
	initPython();
	int *iVar = (int*) malloc(sizeof(int));
	printf("%ld\n", (long) reinterpret_cast<std::uintptr_t>(iVar));

	int pruned_iter = 10;
    std::vector<long> probing_set;
    bool sae_achieved = false;
    int evset_size = 0;
    int pruned_size;

    char *g_mem = (char *) mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    assert(g_mem != MAP_FAILED);
    memset(g_mem, 0xff, mem_size);

    uint64_t *end = (uint64_t *) (g_mem + mem_size);
    uint64_t *ptr;
    int counter = 0;

    for (int iter = 0; iter < pruned_iter; iter++)	{
        if (iter == 0)  {
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
        else    {
            pruned_size = (int) probing_set.size();
            long candidate_set[2 * pruned_size] = {};
            for (int i = 0; i < pruned_size; i++)   {
                candidate_set[i] = probing_set.at(i);
            }
            for (int j = 0; j < pruned_size; j++)
                candidate_set[pruned_size + j] = candidate_set[j];

            int arr[pruned_size];
            callModuleFunc(candidate_set, (sizeof(candidate_set)/sizeof(candidate_set[0])), arr);
            int self_eviction = 0;
            for (int i=0 ; i < sizeof(arr)/sizeof(arr[0]); i++) {
                if(arr[i] > 200)    {
                    probing_set.at(i) = 0;
                    self_eviction++;
                }
            }
            printf("no. of evictions = %d\n", self_eviction);
            probing_set.erase( std::remove (probing_set.begin(), probing_set.end(), 0), probing_set.end() );
            pruned_size = (int) probing_set.size();
            printf("final size = %d\n", pruned_size);
            if (self_eviction < 1)  {
                evset_size = pruned_size;
                break;
            }
            
        }   
    }
    evset_size = pruned_size;
    long eviction_set[2 * evset_size + 1];
    for (int i = 0; i < (2 * evset_size) + 1; ++i)
    {
        if (i == evset_size)
            eviction_set[i] = (long) reinterpret_cast<std::uintptr_t>(iVar);
        else if (i < evset_size)
            eviction_set[i] = probing_set.at(i % evset_size);
        else
            eviction_set[i] = probing_set.at((i - 1) % evset_size);
    }
    int arr[2 * evset_size + 1];
    callModuleFunc(eviction_set, (sizeof(eviction_set)/sizeof(eviction_set[0])), arr);
    int num_eviction = 0;
    for (int i=0 ; i < sizeof(arr)/sizeof(arr[0]); i++) {
        if(arr[i] > 200)    {
            num_eviction++;
        }
    }
    printf("no. of victim evictions = %d\n", num_eviction);
    munmap(g_mem, mem_size);
    return 0;

}

