#!/bin/sh

echo "Executing bucket and ball.."

g++ -o mirage mirage_bucket_and_ball.cc
rm -f mirage_bucket_ball_data.txt

echo "Compilation successful. Running the code.."
times=6

for i in $(seq 0 $times); do
	echo "-------------------------------------------------------------"
	echo "For extra skew $i"
	./mirage $i
	sleep 1
done


python3 plotter.py

