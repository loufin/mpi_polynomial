/*
  COSC 6060
  Matthew Dupont and Louis Finney
  Assignment 2 - MPI work distribution
*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "polynomial.h"

#define NUM_POLYNOMIALS 50000

int main(int argc, char** argv) {
  int rank;
  int numProcs;
	
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

  //Each process generates the full set of coefficients.
  int i;
  printf("Initializing polynomials\n");
  int *coeffArr = (int *)malloc(sizeof(int) * NUM_POLYNOMIALS);
  initialize(coeffArr, NUM_POLYNOMIALS);
  
  /* Start timer */
  double elapsed_time = - MPI_Wtime();
    
  //Processes calculate work as local totals using round robin.
  double localTotal = 0.0;
  for (i = rank; i < NUM_POLYNOMIALS; i += numProcs) {
    //DEBUG printf("LocalTotal:%f\tCoef:%i\tDegree:%i\tVariable:%f\n", localTotal, coeffArr[i], i, VARIABLE);
    localTotal += evaluateTerm(coeffArr[i], i, VARIABLE);
    printf("Thread %i Evaluated Term %i, new local total %f\n", rank, i, localTotal);
  }
  printf("Thread %i\tCompleted - Total %f\n", rank, localTotal);
  
  
  //Aggregate results with reduce
  double result = 0.0;
  MPI_Reduce(&localTotal, &result, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD); 
  if (rank == 0) {
    printf("\n\nGlobal Total: %f\n", result);
  }

  // //Aggregate results with message passing
  // double result;
  // MPI_Status status;
  // if(rank == 0){
  //   for (i = 1; i<numProcs; i++) {
  //     MPI_Recv(&result, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &status);
  //     localTotal += result;
  //   }
  //   printf("\n\nGlobal Total: %f\n", localTotal);
  // } else {
  //   MPI_Send(&localTotal, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
  // }
 
  /* End timer */
  elapsed_time = elapsed_time + MPI_Wtime();
  printf(" sequential value %f wall clock time %8.6f \n", localTotal, elapsed_time);  

  free(coeffArr);
  
  MPI_Finalize();
}
