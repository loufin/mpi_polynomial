/*
  COSC 6060
  Matthew Dupont and Louis Finney
  Assignment 2 - MPI work distribution
*/

#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

#include "polynomial.h"

double sequential(int coeffArr[], int numPolynomials, double variable)
{
  int i;
  double  answer = 0;
  
  printf("Running Sequentially...\n");

  for( i = 0; i < numPolynomials;  i++)
  {
    
    double powerX = power(variable, i);

    //printf("%f ", powerX);
    answer = answer + coeffArr[i] * powerX;
  }
  return answer;
}

double roundRobin(int coeffArr[], int numPolynomials, int rank, int numProcs, double variable) {
  double localTotal = 0.0;
  int i;

  if (rank == 0) printf("Running with Round Robin...\n");

  for (i = rank; i < numPolynomials; i += numProcs) {
    //DEBUG printf("LocalTotal:%f\tCoef:%i\tDegree:%i\tVariable:%f\n", localTotal, coeffArr[i], i, VARIABLE);
    localTotal += evaluateTerm(coeffArr[i], i, variable);
    //DEBUG printf("Thread %i Evaluated Term %i, new local total %f\n", rank, i, localTotal);
  }

  //DEBUG printf("Thread %i\tCompleted - Total %f\n", rank, localTotal);

  return localTotal;
}

double chunk(int coeffArr[], int numPolynomials, int rank, int numProcs, double variable)
{
  int startIndex = (rank * numPolynomials)/numProcs; //divide up array into halves, thirds, fourths, etc. 
  int endIndex = ((rank + 1) * numPolynomials)/numProcs;
  int i;

  if (rank == 0) printf("Running with Chunk...\n");

  //set endIndex to be the last element for the highest rank process 
  if(rank == (numProcs - 1))
      endIndex = numPolynomials;

  double localSum = 0;
  for(i = startIndex; i < endIndex; i++)
  {
    localSum += evaluateTerm(coeffArr[i], i, variable);
  }
  //DEBUG printf("local sum for rank %d is %f \n", rank,localSum);
  
  return localSum;
}

double runSequential(int rank, int numPolynomials) {
  if(rank == 0) {
    //Each process generates the full set of coefficients.
    //DEBUG printf("Initializing polynomials\n");
    int *coeffArr = (int *)malloc(sizeof(int) * numPolynomials);
    initialize(coeffArr, numPolynomials);
    
    /* Start timer */
    double elapsed_time = - MPI_Wtime();

    double localTotal = sequential(coeffArr, numPolynomials, VARIABLE);

    if (rank == 0) {
      printf("\n\nGlobal Total: %f\n", localTotal);
    }

    /* End timer */
    elapsed_time = elapsed_time + MPI_Wtime();
    printf(" sequential value %f wall clock time %8.6f \n", localTotal, elapsed_time);  

    free(coeffArr);

  }
}

double runDistributed(int rank, int numProcs, int numPolynomials, double(*multiThreadStrategy)(int*, int, int, int, double)) 
{
  //Each process generates the full set of coefficients.
  //DEBUG printf("Initializing polynomials\n");
  int *coeffArr = (int *)malloc(sizeof(int) * numPolynomials);
  initialize(coeffArr, numPolynomials);
  
  /* Start timer */
  double elapsed_time = - MPI_Wtime();
    
  //Processes calculate work as local totals using round robin.
  double localTotal = multiThreadStrategy(coeffArr, numPolynomials, rank, numProcs, VARIABLE);

  //Aggregate results with reduce
  double localResult = 0.0;
  MPI_Reduce(&localTotal, &localResult, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD); 

  //result = aggregateMessagePassing(localTotal, rank, numProcs);

  /* End timer */
  elapsed_time = elapsed_time + MPI_Wtime();
  
  double maxElapsedTime;
  MPI_Reduce(&elapsed_time, &maxElapsedTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  if (rank == 0) 
    printf(" sequential value %f wall clock time %8.6f \n", localResult, maxElapsedTime);  

  free(coeffArr);

}

int main(int argc, char** argv) {
  int rank;
  int numProcs;
  
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

  int numPolynomials = 50000;
  if (argc >= 2) {
    numPolynomials = atoi(argv[1]);
  }

  void* executionFunction = roundRobin;
  if (argc >= 3){
    if (!strcmp(argv[2], "-r")) {
      if (rank == 0) printf("Evaluating %i polynomials with %i threads using %s.\n", numPolynomials, numProcs, "Round Robin");
      runDistributed(rank, numProcs, numPolynomials, roundRobin);
    }
    else if (!strcmp(argv[2], "-c")) {
      if (rank == 0) printf("Evaluating %i polynomials with %i threads using %s.\n", numPolynomials, numProcs, "Chunking");
      runDistributed(rank, numProcs, numPolynomials, chunk);
    }
    else {
      if (rank == 0) printf("Evaluating %i polynomials with %i threads using %s.\n", numPolynomials, numProcs, "Sequential");
      runSequential(rank, numPolynomials);
    }
  }

  MPI_Finalize();
}
