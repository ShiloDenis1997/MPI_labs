#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//void MatrixMultiplicationMPI(double *&A, double *&B, double *&C, int &Size);
int **alloc_2d_int(int rows, int cols);
void copyArray(int* destAr, int* sourceAr, int n);

int main(int argc, char** argv)
{
	int size;
	int myrank, leftRank, rightRank;
	int i, j, k;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	int n = atoi(argv[1]); // size of matrix
	int p = atoi(argv[2]); //number of processes
	
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	leftRank = (myrank + n - 1) % n;
	rightRank = (myrank + 1) % n;
	
	int* rowA = (int*)malloc(sizeof(int) * n);
	int* colB = (int*)malloc(sizeof(int) * n);
	int* rowC = (int*)malloc(sizeof(int) * n);
	int* tempSendRow = (int*)malloc(sizeof(int) * n);
	int* tempRecvRow = (int*)malloc(sizeof(int) * n);
	
	MPI_Datatype rowType;
	MPI_Datatype localColType;
	MPI_Datatype colType;
	MPI_Request sendRowRequest = MPI_REQUEST_NULL;
	MPI_Request recvRowRequest = MPI_REQUEST_NULL;
	MPI_Status sendRowStatus;
	MPI_Status recvRowStatus;
	MPI_Type_vector(n, 1, n, MPI_INT, &localColType); // creating column type
	MPI_Type_create_resized(localColType, 0, sizeof(int), &colType);// creating column type
	MPI_Type_commit(&colType);
	
	MPI_Type_vector(1, n, n, MPI_INT, &rowType); // creating row type
	MPI_Type_commit(&rowType);
	
	
	if (!myrank)
	{
		char* filename = argv[3];
		FILE* inputFile;
		int** matrA; 
		int** matrB; 
		matrA = alloc_2d_int(n, n);
		matrB = alloc_2d_int(n, n);
		
		inputFile = fopen(filename, "r");
		for (i = 0; i < n; i++)
		{
			for (j = 0; j < n; j++)
			{
				fscanf(inputFile, "%d", &matrA[i][j]);
			}
		}
		
		for (i = 0; i < n; i++)
		{
			for (j = 0; j < n; j++)
			{
				fscanf(inputFile, "%d", &matrB[i][j]);
			}
		}
		
		MPI_Scatter(matrA[0], 1, rowType, rowA, 1, rowType, 0, MPI_COMM_WORLD);
		MPI_Scatter(matrB[0], 1, colType, colB, 1, rowType, 0, MPI_COMM_WORLD);
		
		free(matrA[0]);
		free(matrA);
		free(matrB[0]);
		free(matrB);
		fclose(inputFile);
	}
	else
	{
		MPI_Scatter(NULL, 1, rowType, rowA, 1, rowType, 0, MPI_COMM_WORLD);
		MPI_Scatter(NULL, 1, colType, colB, 1, rowType, 0, MPI_COMM_WORLD);
	}	
	
	j = myrank;
	for (i = 0; i < n; i++)
	{
		int nextColNum;
		if (i)
		{
			//wait recv row
			MPI_Wait(&recvRowRequest, &recvRowStatus);
			copyArray(colB, tempRecvRow, n);
		}
		
		nextColNum = j - 1;
		if (nextColNum < 0)
		{
			nextColNum += n;
		}
		
		if (i + 1 < n)
		{
			MPI_Irecv(tempRecvRow, 1, rowType, leftRank, nextColNum, MPI_COMM_WORLD, &recvRowRequest);
		}
		
		rowC[j] = 0;
		for (k = 0; k < n; k++)
		{
			rowC[j] += rowA[k] * colB[k];
		}
		
		if (i)
		{
			MPI_Wait(&sendRowRequest, &sendRowStatus);
		}
		
		copyArray(tempSendRow, colB, n);
		if (i + 1 < n)
		{
			MPI_Isend(tempSendRow, 1, rowType, rightRank, j, MPI_COMM_WORLD, &sendRowRequest);
		}
		
		j = nextColNum;
	}
	if (!myrank)
	{
		int** matrC;
		FILE* output;
		matrC = alloc_2d_int(n, n);
		MPI_Gather(rowC, n, MPI_INT, matrC[0], n, MPI_INT, 0, MPI_COMM_WORLD);
		output = fopen("output.txt", "w");
		for (i = 0; i < n; i++)
		{
			for (j = 0; j < n; j++)
			{
				fprintf(output, "%d ", matrC[i][j]);
			}
			
			fprintf(output, "\n");
		}
		free(matrC[0]);
		free(matrC);
	}
	if (myrank)
	{
		MPI_Gather(rowC, n, MPI_INT, NULL, n, MPI_INT, 0, MPI_COMM_WORLD);
	}
	
	MPI_Type_free(&rowType);
	MPI_Type_free(&colType);
	free(tempRecvRow);
	free(tempSendRow);
	free(rowA);
	free(colB);
	free(rowC);
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

void copyArray(int* destAr, int* sourceAr, int n)
{
	int q;
	for (q = 0; q < n; q++)
	{
		destAr[q] = sourceAr[q];
	}
}
