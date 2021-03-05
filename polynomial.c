/*
  COSC 6060
  Matthew Dupont and Louis Finney
  Assignment 2 - MPI work distribution
*/

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <getopt.h>
#include <mpi.h>

#define VARIABLE .99
#define COEFFICIENT 1
#define INDIVIDUALTIMETAG 0

enum executionMode { ex_undefined, ex_sequential, ex_round_robin, ex_chunk };
enum verbosity { v_undefined=-99, v_verbose=10, v_descriptive=0, v_terse2=-10, v_terse=-20 };

//The below are originally taken from the example code for this assignment, with modifications.
double power(double x, int degree)
{     
  if(degree == 0)  return 1;
  
  if(degree == 1)  return x;

  return x * power(x, degree - 1);
}

double evaluateTerm(int coefficient, int degree, double x)
{
  return coefficient * power(x, degree);
}

void initialize(int coeffArr[], int maxDegree)
{
  int i;
  for( i = 0; i < maxDegree; i++)
  {
    coeffArr[i] = COEFFICIENT;
  }
}

//Evaluates the provided coefficient array in a single evaluation, e.g. one process calculates for all of 1..10;
double sequential(int coeffArr[], int numPolynomials, double variable, int verbosity)
{
  int i;
  double answer = 0;
  
  if (verbosity >= v_descriptive) printf("Running Sequentially...\n");

  for( i = 0; i < numPolynomials;  i++)
  {
    answer += evaluateTerm(coeffArr[i], i, variable);
  }
  return answer;
}
//End code originating from assignment

//Evaluates the provided coefficient array in a distributed, round-robin fashin, e.g. 1,4,7,10; 2,5,8; 3,6,9;
double roundRobin(int coeffArr[], int numPolynomials, int rank, int numProcs, double variable, int verbosity) {
  double localTotal = 0.0;
  int i;

  if (rank == 0 && verbosity >= v_descriptive) printf("Using Round Robin strategy...\n");

  for (i = rank; i < numPolynomials; i += numProcs) {
    localTotal += evaluateTerm(coeffArr[i], i, variable);
  }

  if (verbosity >= v_verbose) printf("Thread %i\tCompleted - Total %f\n", rank, localTotal);

  return localTotal;
}


//Chunks the provided coefficient array in distributed contiguous pieces, e.g. 1,2,3,4; 5,6,7; 8,9,10;
double chunk(int coeffArr[], int numPolynomials, int rank, int numProcs, double variable, int verbosity)
{
  int startIndex = (rank * numPolynomials)/numProcs; //divide up array into halves, thirds, fourths, etc. 
  int endIndex = ((rank + 1) * numPolynomials)/numProcs;
  int i;

  if (rank == 0 && verbosity >= v_descriptive) printf("Using Chunk strategy...\n");

  //set endIndex to be the last element for the highest rank process 
  if(rank == (numProcs - 1))
      endIndex = numPolynomials;

  double localSum = 0;
  for(i = startIndex; i < endIndex; i++)
  {
    localSum += evaluateTerm(coeffArr[i], i, variable);
  }
  if (verbosity >= v_verbose) printf("local sum for rank %d is %f \n", rank,localSum);
  
  return localSum;
}

//Generates a set of coefficients for a polynomial then evaluates the polynomial sequentially.
//Ignores all spawned processes besides that of rank 0.
double runSequential(int rank, int numPolynomials, double variable, int verbosity) {
  if(rank == 0) {
    //Each process generates the full set of coefficients.
    int *coeffArr = (int *)malloc(sizeof(int) * numPolynomials);
    initialize(coeffArr, numPolynomials);
    
    /* Start timer */
    double elapsed_time = - MPI_Wtime();

    double localTotal = sequential(coeffArr, numPolynomials, variable, verbosity);

    /* End timer */
    elapsed_time = elapsed_time + MPI_Wtime();

    //Format output
    if (rank == 0) {
      switch(verbosity) {
        case v_terse2: {
          printf("%i,%i,%f,%f,%i,%f\n", 1, numPolynomials, variable, localTotal, 0, elapsed_time);
          break;
        }
        case v_terse: {
          printf("%i,%i,%f,%f,%f\n", 1, numPolynomials, variable, localTotal, elapsed_time);
          break;
        }
        default: {
          printf("Final Total: %f\tWall clock time %8.6\t\n", localTotal, elapsed_time);
          break;
        }
      }
    }  
    free(coeffArr);
    return localTotal;
  }
}

//Populates the provided buffer with timings from each proc, with index = procnum.
void aggregateDetailedTimings(int rank, int numProcs, double elapsed_time, double* buffer) {
  MPI_Status status;
  int i;
  if (rank == 0) {
    buffer[0] = elapsed_time;
    for (i = 1; i < numProcs; i++) {
      MPI_Recv(&buffer[i], 1, MPI_DOUBLE, i, INDIVIDUALTIMETAG, MPI_COMM_WORLD, &status);
    }
  } else {
    MPI_Send(&elapsed_time, 1, MPI_DOUBLE, 0, INDIVIDUALTIMETAG, MPI_COMM_WORLD);
  }
}

//Generates a set of coefficients for a polynomial then evaluates the polynomial using a passed multiThread Strategy
double runDistributed(int rank, int numProcs, int numPolynomials, double variable, double(*multiThreadStrategy)(int*, int, int, int, double, int), int verbosity) 
{
  //Each process generates the full set of coefficients.
  int *coeffArr = (int *)malloc(sizeof(int) * numPolynomials);
  initialize(coeffArr, numPolynomials);
  int i;

  /* Start timer */
  double elapsed_time = - MPI_Wtime();

  double localTotal = multiThreadStrategy(coeffArr, numPolynomials, rank, numProcs, variable, verbosity);

  /* End timer */
  elapsed_time = elapsed_time + MPI_Wtime();

  if (verbosity >= v_verbose) printf("Thread %i time taken: %f\n", rank, elapsed_time);
  //Aggregate results with reduce
  double localResult = 0.0;
  MPI_Reduce(&localTotal, &localResult, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD); 
  
  double maxElapsedTime;
  double avgElapsedTime;
  MPI_Reduce(&elapsed_time, &maxElapsedTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&elapsed_time, &avgElapsedTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  avgElapsedTime = avgElapsedTime/numProcs;

  double* times = (double *)malloc(sizeof(double) * numProcs);
  bzero(times, sizeof(double) * numProcs);

  if (verbosity == v_terse2) { 
    aggregateDetailedTimings(rank, numProcs, elapsed_time, times); 
  }

  if (rank == 0) {
    switch(verbosity){
      case v_terse: {
        printf("%i,%i,%f,%f,%f\n", numProcs, numPolynomials, variable, localResult, maxElapsedTime);
        break;
      }
      case v_terse2: {
        for (i = 0; i < numProcs; i++) {
          printf("%i,%i,%f,%f,%i,%f\n", numProcs, numPolynomials, variable, localResult, i, times[i]);
        }
        break;
      }
      case v_verbose: {
        printf("Final Total: %f\tWall clock time %8.6f\tAverage Time: %f\n", localResult, maxElapsedTime, avgElapsedTime);  
        break;
      }
      case v_descriptive: {} //Fall through to default.
      default: {
        printf("Final Total: %f\tWall clock time %8.6\t\n", localResult, maxElapsedTime);
        break;
      }
    }
  }

  free(times);
  free(coeffArr);
  return localTotal;
}

void showUsage(char* arg0) {
  printf("\n------------------------------------------------------------------\n");
  printf("Calculates the value of a polynomial with -p terms (default 50000),\n");
  printf("with coefficients of 1 and variable of -v (default .99). Exponents \n");
  printf("correspond to coefficient array indices.\n\n");
  printf("Usage: %s [OPTION]\n", arg0);
  printf("  -h, --help\n");
  printf("    Displays information about how to use this function.\n");
  printf("  -v, --variable\n");
  printf("    The variable to be multiplied in the polynomial. Default .99.\n");
  printf("  -p, --polySize\n");
  printf("    The length of the polynomial in terms. Default 50000.\n");
  printf("  -S, --strategy\n");
  printf("    The strategy to use. Options are \"round_robin\", \"chunk\",\n");
  printf("    and \"sequential\". Default sequential.\n");
  printf("    Mutually exclusive with -r, -c, -s.\n");
  printf("  -r, --round_robin\n");
  printf("    Use the Round-robin distributed strategy. \n");
  printf("    Mutually exclusive with -S, -c, -s.\n");
  printf("  -c, --chunk\n");
  printf("    Use the chunk distributed strategy. \n");
  printf("    Mutually exclusive with -S, -r, -s.\n");
  printf("  -s, --sequential\n");
  printf("    Use the sequential distributed strategy. Default. \n");
  printf("    Mutually exclusive with -S, -r, -s.\n");
  printf("  -v|-d|-t|-y, --verbose|--descriptive|--terse|--terse2\n");
  printf("    Specify verbosity, from verbose to terse. \n");
  printf("    Default is --descriptive.\n");
  printf("    --verbose is intended for debugging and additional logging.\n");
  printf("    --terse produces only csv-ready lines, with maximum clock time.\n");
  printf("    --terse2 produces only csv-ready lines, with clock times per \n");
  printf("      processor.\n");
  printf("    Mutually exclusive with each other.\n");
  printf("\n------------------------------------------------------------------\n");
}

// Validates the parameter passed to -S matches allowed names. 
// Sets error flag if it fails.
int matchExecution(char* argument, int rank, int* error) {
  int execution;
  if(!strcmp(argument, "round_robin")){
    execution = ex_round_robin;
  }
  else if(!strcmp(argument, "chunk")){
    execution = ex_chunk;
  }
  else if(!strcmp(argument, "sequential")){
    execution = ex_sequential;
  }
  else{
    if (rank == 0) printf("Unknown strategy: %s\n", argument);
    *error = 1;
  }
  return execution;
}

// Calculates the value of a polynomial with -p terms (default 50000),
// with coefficients of 1 and variable of -v (default .99). Exponents
// correspond to coefficeint array indices.
int main(int argc, char** argv) {
  int rank;
  int numProcs;
  
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

  int numPolynomials = 50000;
  double variable = .99;
  int execution = ex_undefined;
  int verbosity = v_undefined;

  static struct option longOpts[] = 
    {
      // This does involve setting values in getOpt, which is thread unsafe, 
      // but MPI should be multi-process and thus this shouldn't be a concern.
      // See https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Options.html

      //long option,    argument status,    flag, short option
      { "help",	        no_argument,	      0,    'h' },
      { "poly_size",	  required_argument,	0,    'p' },
      { "variable",	    required_argument,  0,    'x' },
      { "strategy",	    required_argument,	0,    'S' },
      { "round_robin",  no_argument,	      0,    'r' },
      { "chunk",	      no_argument,        0,    'c' },
      { "sequential",	  no_argument,        0,    's' },
      { "verbose",	    no_argument,        0,    'v' },
      { "descriptive",  no_argument,        0,    'd' },
      { "terse",	      no_argument,        0,    't' },
      { "terse2",	      no_argument,        0,    'y' }
    };

  int c;
  int optionIndex = 0;
  int error = 0;

  while (error == 0 && (c = getopt_long(argc, argv, "hp:x:S:rcsvdty", longOpts, &optionIndex)) > -1){
    switch(c){
      //Usage
      case 'h':
        error = 1;
        break;
      
      //Parameters
      case 'p':
        numPolynomials = atoi(optarg);
        break;
      case 'x':
        variable = atof(optarg);
        break;

      //RunModes
      case 'S':
        if(execution == ex_undefined) { execution = matchExecution(optarg, rank, &error); }
        else { 
          if (rank == 0) printf("Error: Multiple execution modes set\n");
          error = 1;
        }
        break;
      case 'r':
        if(execution == ex_undefined) { execution = ex_round_robin; }
        else { 
          if (rank == 0) printf("Error: Multiple execution modes set\n");
          error = 1;
        }
        break;
      case 'c':
        if(execution == ex_undefined) { execution = ex_chunk; }
        else {
          if (rank == 0) printf("Error: Multiple execution modes set\n");
          error = 1;
        }
        break;
      case 's':
        if(execution == ex_undefined) { execution = ex_sequential; }
        else {
          if (rank == 0) printf("Error: Multiple execution modes set\n");
          error = 1;
        }
        break;

      //Messaging
      case 'v':
        if(verbosity == v_undefined) { verbosity = v_verbose; }
        else {
          if (rank == 0) printf("Error: Multiple verbosity modes set\n");
          error = 1;
        }
        break;
      case 'd':
        if(verbosity == v_undefined) { verbosity = v_descriptive; }
        else {
          if (rank == 0) printf("Error: Multiple verbosity modes set\n");
          error = 1;
        }
        break;
      case 't':
        if(verbosity == v_undefined) { verbosity = v_terse; }
        else {
          if (rank == 0) printf("Error: Multiple verbosity modes set\n");
          error = 1;
        }
        break;
      case 'y':
        if(verbosity == v_undefined) { verbosity = v_terse2; }
        else {
          if (rank == 0) printf("Error: Multiple verbosity modes set\n");
          error = 1;
        }
        break;

      //Fallthrough
      default:
        error = 1;
        break;
    }
  }

  if (error != 0) {
    if (rank == 0) showUsage(argv[0]);
    MPI_Finalize();
    return 0;
  }

  //Specify default execution parameters.
  if (execution == ex_undefined) execution = ex_sequential;
  if (verbosity == v_undefined) verbosity = v_descriptive;

  switch (execution) {
    case ex_sequential:
    if (rank == 0 && verbosity >= v_descriptive) printf("Evaluating %i polynomials with exponent %f sequentially.\n", numPolynomials, variable);
      runSequential(rank, numPolynomials, variable, verbosity);
      break;
    case ex_round_robin:
      if (rank == 0 && verbosity >= v_descriptive) printf("Evaluating %i polynomials with exponent %f using %i processes.\n", numPolynomials, variable, numProcs);
      runDistributed(rank, numProcs, numPolynomials, variable, roundRobin, verbosity);
      break;
    case ex_chunk:
      if (rank == 0 && verbosity >= v_descriptive) printf("Evaluating %i polynomials with exponent %f using %i processes.\n", numPolynomials, variable, numProcs);
      runDistributed(rank, numProcs, numPolynomials, variable, chunk, verbosity);
      break;
    default:
      if (rank == 0) showUsage(argv[0]);
      break;
  }

  MPI_Finalize();
  return 0;
}
