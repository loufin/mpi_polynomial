/* 
Matthew Dupont and Louis Finney 
COSC 6060
*/ 
#include<mpi.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

#define COLUMNS 10
#define ROWS 10

void populateMatrix(int matrix[][COLUMNS])
{
  int row;
  int col;

  for(row = 0; row < ROWS; row++)
  {
    for(col = 0; col < COLUMNS; col++)
    {
          matrix[row][col] = 1;
    }
  }
}

int distributeSum(int matrix[ROWS][COLUMNS], int rank, int numProcs){
  int row;
  int col = rank%numProcs; 
  int total = 0;
  int startCol;
  for(row = 0; row < ROWS; row++){
    startCol = abs((rank+numProcs-row)%numProcs);
    for(col = startCol; col < COLUMNS; col = col + numProcs)
    {
      total = total + matrix[row][col];
      //DEBUG printf("Rank %d adding row %d column %d for running total %d \n", rank, row, col, total);
    }
  }
  //MPI_Send(&total, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
  //DEBUG printf("total for rank %d is %d \n", rank, total);
  return total;
}

int sequentialSum(int matrix[][COLUMNS])
{
  int row;
  int col;
  int total = 0;

  for(row = 0; row < ROWS; row++)
  {
    for(col = 0; col < COLUMNS; col++)
    {
         total = total +  matrix[row][col];
    }
  }
 return total;
}

int main(int argc, char **argv)
{
  int nprocs;
  int rank;

  MPI_Init(&argc,&argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  
  int matrix[ROWS][COLUMNS];
  populateMatrix(matrix);
  
  int seq_total;
  int global_sum;
  int local_sum = 0;
  local_sum = distributeSum(matrix, rank, nprocs);
  //DEBUG printf("local sum for rank %d is %d \n", rank,global_sum);
  MPI_Status status;
  MPI_Reduce(&local_sum, &global_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);  
  if(rank == 0)
  {
     //seq_total = sequentialSum(matrix);
     //printf("sequential sum is %d \n", seq_total);
     printf("distributed sum is %d \n", global_sum);
  }
  
  
  
  
  MPI_Finalize();
  return 0;
}
