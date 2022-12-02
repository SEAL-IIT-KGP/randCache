#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri Jul  1 12:16:04 2022

@author: anirban
"""


import random 
import os
import numpy as np
import matplotlib.pyplot as plt


outfile = "outfile.txt"
eviction_file = "eviction_status.txt"
if os.path.exists(outfile):
    os.remove(outfile)

with open("config.ini", "r") as f:
    lines = f.readlines()

throws_list = [80000, ]

for j in range(1):
    if os.path.exists(eviction_file):
        os.remove(eviction_file)
    with open("config.ini", "w") as f:
        for line in lines:
            if line.strip("\n").startswith("cache-size"):
                f.write("num-additional-tags="+str(j))
            else:
                f.write(line)
    num = []
    for i in range(80000):
        num.append(random.randint(0, 100000000))
    
    with open('address_list.txt', 'w') as filehandle:
        filehandle.writelines(str(num))
    os.system("python3 main.py")
    
    
#    
#with open('outfile.txt', 'r') as f:
#    file = f.readlines()
#
#count = list(map(lambda each:each.strip("\n"), file))
#
#
#valid_count = []
#counter = 0
#for i in range(len(count)):
#    if (count[i] == ""):
#        valid_count.append(counter)
#        counter = 0
#    else:
#        counter += 1
#        
#        
#print(np.mean(valid_count))
#X = []
#for i in range(len(valid_count)):
#    X.append(i)
#
#plt.bar(X, valid_count)
#plt.show()