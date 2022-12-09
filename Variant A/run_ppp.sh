#!/bin/bash

g++ -I/usr/include/python3.8 -o evset1 eviction_set_v1.cpp -lpython3.8
sleep 1
./evset1
sleep 1
python3 getHM_ppp.py
