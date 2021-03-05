#!/bin/bash

mpicc -o polynomial.exe polynomial.c

rm -r -f "results"
mkdir "results"
for file in results/sequential.csv results/roundRobin.csv results/chunk.csv; do
  echo "Procs,Terms,Variable,Total,MaxTime" > $file
done

for polysize in 50000 75000 100000; do
  mpirun -np 1 polynomial.exe --terse --sequential -p $polysize >> results/sequential.csv
  for np in $(seq 1 16); do
    mpirun -np $np polynomial.exe --terse --round_robin -p $polysize >> results/roundRobin.csv
    mpirun -np $np polynomial.exe --terse --chunk -p $polysize >> results/chunk.csv
  done
  echo "Finished evaluating for $polysize terms for verbose output"
done


