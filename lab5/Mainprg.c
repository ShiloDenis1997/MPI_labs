#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int **alloc_2d_int(int rows, int cols);

int main(int argc, char** argv)
{
	int size;
	int myrank;
	int tag = 10;
	int i, j;
	int col_counter;
	int n1, n2;
	int line[100];
	int** commBuffer;
	int** resultBuffer;
	int* prevRow;
	int row_block_size;
	int col_block_size;
	int r;
	int number_count;
	MPI_Status status;

	MPI_Init(&argc, &argv);

	n1 = atoi(argv[1]);
	n2 = atoi(argv[2]);
	r = atoi(argv[3]);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);


	col_block_size = n2 / r;
	row_block_size = n1 / size;
	
	
	MPI_Request sendRowRequest = MPI_REQUEST_NULL;
	MPI_Request getRowRequest = MPI_REQUEST_NULL;
	MPI_Status sendRowStatus;
	MPI_Status getRowStatus;
	int i1, j1;
	//printf("myrank: %d block size: %d\n", myrank, row_block_size);
	prevRow = (int*)malloc(n2*sizeof(int));
	if (!myrank)
	{
		for (i = 0; i < col_block_size; i++)
		{
			prevRow[i] = rand() % 50;
		}
	}
	
	resultBuffer = alloc_2d_int(row_block_size, n2);
	if (myrank)
		MPI_Irecv(&prevRow[0], col_block_size, MPI_INT, myrank-1, 0, MPI_COMM_WORLD, &getRowRequest);
	for (i = 0; i < n2; i+=col_block_size)
	{
		if (myrank)
		{
			MPI_Wait(&getRowRequest, &getRowStatus);
			MPI_Irecv(&prevRow[i + col_block_size], col_block_size, MPI_INT, myrank-1, i + col_block_size, MPI_COMM_WORLD, &getRowRequest);
		}
		else
			for (j = i; j < i + col_block_size; j++)
			{
				prevRow[j] = rand() % 50;
			}
		//printf("process %d ", myrank);
		for (j = 0; j < col_block_size; j++)
		{
			//printf("%d ", prevRow[i + j]);
			resultBuffer[0][i + j] = prevRow[i + j] + 1;
		}
		//printf("\n");
		
		
		for (i1 = 1; i1 < row_block_size; i1++)
			for (j1 = 0; j1 < col_block_size; j1++)
			{
				resultBuffer[i1][i + j1] = resultBuffer[i1 - 1][i + j1] + 1;
			}
		
		
		//send row part to the next process
		if (myrank != size - 1)
		{
			//printf("Sending from process %d from position %d\n", myrank, i);
			// for (j = i; j < i + col_block_size; j++)
				// printf("%d ", resultBuffer[row_block_size - 1][j]);
			// printf("\n");
			MPI_Isend(&resultBuffer[row_block_size - 1][i], col_block_size, MPI_INT, myrank + 1, i, MPI_COMM_WORLD, &sendRowRequest);
		}
	}
	if (myrank != size - 1)
	{
		MPI_Wait(&sendRowRequest, &sendRowStatus);
	}
	
	printf("data on process %d\n", myrank);
	for (i = 0; i < row_block_size; i++)
	{
		for (j = 0; j < n2; j++)
		{
			printf("%d ", resultBuffer[i][j]);
		}
		printf("\n");
	}
	free(resultBuffer[0]);
	free(resultBuffer);
	free(prevRow);
	
	MPI_Finalize();
	return 0;
}

int **alloc_2d_int(int rows, int cols) {
	int i;
    int *data = (int *)malloc(rows*cols*sizeof(int));
    int **array= (int **)malloc(rows*sizeof(int*));
    for (i=0; i<rows; i++)
        array[i] = &(data[cols*i]);

    return array;
}
