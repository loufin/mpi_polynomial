#!/bin/bash

mpicc -o polynomial.exe polynomial.c

rm -r -f "results_2"
mkdir "results_2"
for file in results_2/sequential.csv results_2/roundRobin.csv results_2/chunk.csv; do
  echo "Procs,Terms,Variable,Total,Rank,Time" > $file
done

for polysize in 50000 75000 100000; do
  mpirun -np 1 polynomial.exe --terse --sequential -p $polysize >> results_2/sequential.csv
  for np in $(seq 1 16); do
    mpirun -np $np polynomial.exe -y --round_robin -p $polysize >> results_2/roundRobin.csv
    mpirun -np $np polynomial.exe -y --chunk -p $polysize >> results_2/chunk.csv
  done
  echo "Finished evaluating for $polysize terms for detailed verbose output"
done