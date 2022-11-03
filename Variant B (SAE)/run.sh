#!/bin/bash

g++ -I/usr/include/python3.8 -o collision finding_collision.cpp -lpython3.8

for i in {1..20}
do
   ./collision
	sleep .5
	a='address_list_'
	b='cache_details_'
	c="${a}${i}"
	d="${b}${i}"
	mv address_list.txt collision_data/${c}.txt
	mv cache_details.txt collision_data/${d}.txt
	sleep .5
   	echo "[x] end of $i iteration"
done
echo "All done..............!"