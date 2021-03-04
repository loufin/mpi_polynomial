/*
  COSC 6060
  Matthew Dupont and Louis Finney
  Assignment 2 - MPI work distribution
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "polynomial.h"

double calculateTotal(int* polynomials, int numPolynomials, int rank, int numProcs, double variable) {
  double localTotal = 0.0;
  int i;
  for (i = rank; i < numPolynomials; i += numProcs) {
    //DEBUG printf("LocalTotal:%f\tCoef:%i\tDegree:%i\tVariable:%f\n", localTotal, coeffArr[i], i, VARIABLE);
    localTotal += evaluateTerm(polynomials[i], i, variable);
    //DEBUG printf("Thread %i Evaluated Term %i, new local total %f\n", rank, i, localTotal);
  }
  printf("Thread %i\tCompleted - Total %f\n", rank, localTotal);
  return localTotal;
}

double aggregate_message_passing(double localTotal, int rank, int numProcs) {
  //Aggregate results with message passing
  double result;
  int i;
  MPI_Status status;
  
  if(rank == 0){
    for (i = 1; i<numProcs; i++) {
      MPI_Recv(&result, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &status);
      localTotal += result;
    }
    return localTotal;
  } else {
    MPI_Send(&localTotal, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
    return 0.0;
  }
}

int main(int argc, char** argv) {
  int rank;
  int numProcs;

  int numPolynomials = 50000;
  if (argc == 2) {
    numPolynomials = atoi(argv[1]);
  }

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

  //Each process generates the full set of coefficients.
  printf("Initializing polynomials\n");
  int *coeffArr = (int *)malloc(sizeof(int) * numPolynomials);
  initialize(coeffArr, numPolynomials);
  
  /* Start timer */
  double elapsed_time = - MPI_Wtime();
    
  //Processes calculate work as local totals using round robin.
  double localTotal = calculateTotal(coeffArr, numPolynomials, rank, numProcs, VARIABLE);
  
  //Aggregate results with reduce
  double result = 0.0;
  //MPI_Reduce(&localTotal, &result, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD); 
  result = aggregate_message_passing(localTotal, rank, numProcs);

  if (rank == 0) {
    printf("\n\nGlobal Total: %f\n", result);
  }

  /* End timer */
  elapsed_time = elapsed_time + MPI_Wtime();
  printf(" sequential value %f wall clock time %8.6f \n", localTotal, elapsed_time);  

  free(coeffArr);
  
  MPI_Finalize();
}
