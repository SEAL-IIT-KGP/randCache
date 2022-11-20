#!/bin/bash

g++ -I/usr/include/python3.8 -o evset4 eviction_set_v4.cpp -lpython3.8
python3 getHM.py