#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Fri May 28 14:29:38 2021

@author: anirban
"""
import matplotlib.pyplot as plt
import numpy as np


with open("timing_nopart_norand.txt", 'r') as f1:
    file = f1.readlines()
    
timing = list(filter(None, list(map(lambda each:each.strip("\n"), file))))

count_hits = [0] * len(timing)
for i, rows in enumerate(timing):
    rows = list(map(int, rows.split()))
    count_hits[i] = 0
    for item in rows:
        count_hits[i] += 1
    timing[i] = rows

count_hits = list(filter(lambda x: x > 0, count_hits))
    

index = []
for i in range(1, len(count_hits) + 1):
    index.append(i)



fig=plt.figure(figsize=(8, 2))
ax = plt.gca()
plt.bar(index, count_hits, width=0.5)
plt.xlabel("probing locations", fontweight='bold', fontsize=12)
plt.ylabel("Number of missed", fontweight='bold', fontsize=12)
ax.xaxis.set_tick_params(labelsize=11)
ax.yaxis.set_tick_params(labelsize=11)
plt.xticks(weight = 'bold')
plt.yticks(weight = 'bold')
plt.savefig("Type-I cache.pdf", dpi=1200, bbox_inches = 'tight')
plt.show()
