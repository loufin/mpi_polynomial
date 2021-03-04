/*
  COSC 6060
  Matthew Dupont and Louis Finney
  Assignment 2 - MPI work distribution
*/

#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>
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

  if (rank == 0) printf("Using Round Robin strategy...\n");

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

  if (rank == 0) printf("Using Chunk strategy...\n");

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

double runSequential(int rank, int numPolynomials, double variable) {
  if(rank == 0) {
    //Each process generates the full set of coefficients.
    //DEBUG printf("Initializing polynomials\n");
    int *coeffArr = (int *)malloc(sizeof(int) * numPolynomials);
    initialize(coeffArr, numPolynomials);
    
    /* Start timer */
    double elapsed_time = - MPI_Wtime();

    double localTotal = sequential(coeffArr, numPolynomials, variable);

    /* End timer */
    elapsed_time = elapsed_time + MPI_Wtime();
    printf(" sequential value %f wall clock time %8.6f \n", localTotal, elapsed_time);  

    free(coeffArr);

  }
}

double runDistributed(int rank, int numProcs, int numPolynomials, double variable, double(*multiThreadStrategy)(int*, int, int, int, double)) 
{
  //Each process generates the full set of coefficients.
  //DEBUG printf("Initializing polynomials\n");
  int *coeffArr = (int *)malloc(sizeof(int) * numPolynomials);
  initialize(coeffArr, numPolynomials);
  
  /* Start timer */
  double elapsed_time = - MPI_Wtime();
    
  //Processes calculate work as local totals using round robin.
  double localTotal = multiThreadStrategy(coeffArr, numPolynomials, rank, numProcs, variable);

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

void showUsage(char* arg0) {
  printf("Calculates the value of a polynomial with -p terms (default 50000),\n");
  printf("with coefficients of 1 and variable of -v (default .99). Exponents \n");
  printf("correspond to coefficeint array indices.\n\n");
  printf("Usage: %s [OPTION]\n", arg0);
  printf("  -h, --help\n");
  printf("    Displays information about how to use this function.\n");
  printf("  -v, --variable\n");
  printf("    The variable to be multiplied in the polynomial. Default .99.\n");
  printf("  -p, --polySize\n");
  printf("    The length of the polynomial in terms. Default 50000.\n");
  printf("  -s, --strategy\n");
  printf("    The strategy to use. Options are \"roundRobin\", \"chunk\",\n");
  printf("    and \"sequential\". Default sequential.\n");
}


int main(int argc, char** argv) {
  int rank;
  int numProcs;
  
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

  int numPolynomials = 50000;
  double variable = 0.99;
  int execution = 0; //0 = runSequential, 1 = runDistributed
  void* strategy = roundRobin;

  static struct option longOpts[] = 
    {
      // This does involve setting values in getOpt, which is thread unsafe, 
      // but MPI should be multi-process and thus this shouldn't be a concern.
      // See https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Options.html

      //long option,  argument status,    flag, short option
      { "help",	      no_argument,	      0,	  'h' },
      { "polySize",	  required_argument,	0,	  'p' },
      { "variable",	  required_argument,  0,	  'v' },
      { "strategy",	  required_argument,	0,	  's' }
    };
  int c;
  int optionIndex = 0;
  while (execution >= 0 && (c = getopt_long(argc, argv, "hp:v:s:", longOpts, &optionIndex)) > -1){
    switch(c){
      case 'h':
        execution = -1;
        break;
      case 'p':
        numPolynomials = atoi(optarg);
        break;
      case 'v':
        variable = atof(optarg);
        break;
      case 's':
        if(!strcmp(optarg, "roundRobin")){
          execution = 1;
          strategy = roundRobin;
          break;
        }
        else if(!strcmp(optarg, "chunk")){
          execution = 1;
          strategy = chunk;
          break;
        }
        else if(!strcmp(optarg, "sequential")){
          execution = 0;
          break;
        }
        else{
          if (rank == 0) printf("Unknown strategy: %s\n", optarg);
          execution = -1;
          break;
        }
      default:
        execution = -1;
        break;
    }
  }

  // if (argc >= 2) {
  //   numPolynomials = atoi(argv[1]);
  // }

  // if (argc >= 3){
  //   if (!strcmp(argv[2], "-r")) {
  //     if (rank == 0) printf("Evaluating %i polynomials with %i threads using %s.\n", numPolynomials, numProcs, "Round Robin");
  //     runDistributed(rank, numProcs, numPolynomials, roundRobin);
  //   }
  //   else if (!strcmp(argv[2], "-c")) {
  //     if (rank == 0) printf("Evaluating %i polynomials with %i threads using %s.\n", numPolynomials, numProcs, "Chunking");
  //     runDistributed(rank, numProcs, numPolynomials, chunk);
  //   }
  //   else {
  //     if (rank == 0) printf("Evaluating %i polynomials with %i threads using %s.\n", numPolynomials, numProcs, "Sequential");
  //     runSequential(rank, numPolynomials);
  //   }
  // }
  char* functionName = "polynomial.exe";
  switch (execution) {
    case 0:
    if (rank == 0) printf("Evaluating %i polynomials with exponent %f sequentially.\n", numPolynomials, variable);
      runSequential(rank, numPolynomials, variable);
      break;
    case 1:
      if (rank == 0) printf("Evaluating %i polynomials with exponent %f using %i processes.\n", numPolynomials, variable, numProcs);
      runDistributed(rank, numProcs, numPolynomials, variable, strategy);
      break;
    case -1:
      if(argc > 0) functionName = argv[0];
      if (rank == 0) showUsage(functionName);
      break;
    default:
      break;
  }

  MPI_Finalize();
}
