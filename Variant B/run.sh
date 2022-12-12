#!/bin/bash

g++ -I/usr/include/python3.8 -o evset eviction_set_mirage_v1.cpp -lpython3.8
sleep 1
./evset
sleep 1
python3 getHM.py