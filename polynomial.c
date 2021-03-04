#include<stdio.h>
#include<stdlib.h>
#include<mpi.h>

#define MAX 50000
#define COEFFICIENT 1


double power(double x, int degree)
{     
      if(degree == 0)  return 1;
      
      if(degree == 1)  return x;

      return x * power(x, degree - 1);
}

double sequential(int coeffArr[], double x)
{
   int maxDegree = MAX - 1;
   int i;
   double  answer = 0;
   
   for( i = 0; i < maxDegree;  i++)
   {
      
      double powerX = power(x, i);

      //printf("%f ", powerX);
      answer = answer + coeffArr[i] * powerX;
   }
   return answer;
}

double chunk(int coeffArr[], double x, int rank, int numProcs)
{
   
   // SEND SIDE 

   int startIndex = (rank * MAX)/numProcs; //divide up array into halves, thirds, fourths, etc. 
   int endIndex = ((rank + 1) * MAX)/numProcs;
 
   //set endIndex to be the last element for the highest rank process 
   if(rank == (numProcs - 1))
        endIndex = MAX;

   double localSum = 0;
   for(int i = startIndex; i < endIndex; i++)
   {
      localSum = localSum + power(coeffArr[i], x);
   }
   //DEBUG printf("local sum for rank %d is %f \n", rank,localSum);
   
   // RECEIVE SIDE 
   int source = 0; 
   MPI_Status status;
   double final = localSum; //so we don't have to send it if rank = 0
   if(rank == 0){
      int source; 
      double sum;
      final = localSum;
      for (source = 1; source < numProcs; source++)
      {
         MPI_Recv(&sum, 1, MPI_DOUBLE, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
         final = final + sum;
      }
      
   }
   else{
      int destination = 0;
      MPI_Send(&localSum, 1, MPI_DOUBLE, destination, 1, MPI_COMM_WORLD);
   }
   return final; 
}

double roundRobin(int coeffArr[], double x)
{

   return 0;
}

void initialize(int coeffArr[])
{
   int maxDegree = MAX - 1;
   int i;
   for( i = 0; i < maxDegree; i++)
   {
      coeffArr[i] = COEFFICIENT;
   }
}

// Driver Code
int main(int argc, char **argv)
{
    
    int *coeffArr = (int *)malloc(sizeof(int) * MAX);
    
    initialize(coeffArr);
    double x = 0.99;
    
    MPI_Init (&argc, &argv);
    int rank;
    int numProcs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // rank is process id
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs); // number of processes in total, from command line 
    

    /* Start timer */
    double elapsed_time = - MPI_Wtime();
    
    //double value = sequential(coeffArr, x);
    double value = chunk(coeffArr, x, rank, numProcs);
    /* End timer */
    elapsed_time = elapsed_time + MPI_Wtime();
   
   if(rank == 0){
      printf(" sequential value %f wall clock time %8.6f \n", value, elapsed_time);
   }
    
    fflush(stdout);
    
    free(coeffArr);
    
    MPI_Finalize();
    
	return 0;
}
