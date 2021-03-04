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

double chunk(int coeffArr[], double x)
{
   int rank;
   int numProcs;
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // rank is process id
   MPI_Comm_size(MPI_COMM_WORLD, &numProcs); // number of processes in total, from command line 

   // SEND SIDE 
   int destination;

   //
   int startIndex = (rank * MAX)/numProcs;
   int endIndex = ((rank + 1) * MAX)/numProcs;
 
   //If the endIndex
   if(rank == (numProcs - 1))
        endIndex = MAX;

   double localSum = 0;
   for(int i = startIndex; i < endIndex; i++)
   {
      localSum = localSum + power(coeffArr[i], x);
   }
   MPI_Send(&localSum, 1, MPI_DOUBLE, rank, 1, MPI_COMM_WORLD);
   
   // RECEIVE SIDE 
   int receivedCode; 
   int source = 0; 
   MPI_Status status;
   double recv;
   double final = 0;
   int tag;
   if(rank == 0){
      int source; 
      double sum;
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
    
    chunk(coeffArr, x);

    /* Start timer */
    double elapsed_time = - MPI_Wtime();
    
    double value = sequential(coeffArr, x);
    
    /* End timer */
    elapsed_time = elapsed_time + MPI_Wtime();
        
    printf(" sequential value %f wall clock time %8.6f \n", value, elapsed_time);
    fflush(stdout);
    
    free(coeffArr);
    
    MPI_Finalize();
    
	return 0;
}
