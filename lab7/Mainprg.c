#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//void MatrixMultiplicationMPI(double *&A, double *&B, double *&C, int &Size);
int **alloc_2d_int(int rows, int cols);
void copyArray(int* destAr, int* sourceAr, int n);
int countLeftProcessRank(int myrank, int processCount)
{
	return (myrank + processCount - 1) % processCount;
}

int countRightProcessRank(int myrank, int processCount)
{
	return (myrank + 1) % processCount;
}

void printMatrix(int** matr, int rowsCount, int colsCount)
{
	int i, j;
	for (i = 0; i < rowsCount; i++)
	{
		for (j = 0; j < colsCount; j++)
		{
			printf("%d", matr[i][j]);
		}
		printf("\n");
	}
}

int main(int argc, char** argv)
{
	int myrank;
	int i, j, k;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	int rowsCountA = atoi(argv[1]); // count of rows in A
	int colsCountA = atoi(argv[2]); // count of columns in A
	int rowsCountB = colsCountA; // count of rows in B
	int colsCountB = rowsCountA; // count of columns in B
	int procCount = atoi(argv[3]); // count of processes
	
	int rowsInRowBlock, rowsInColBlock, colsInRowBlock, colsInColBlock;
	int elementsInRowBlockCount = rowsInRowBlock * colsInRowBlock;
	int elementsInColBlockCount = rowsInColBlock * colsInColBlock;
	int leftProcessRank, rightProcessRank;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	leftProcessRank = countLeftProcessRank(myrank, procCount);
	rightProcessRank = countRightProcessRank(myrank, procCount);
	
	rowsInRowBlock = rowsCountA / procCount;
	colsInRowBlock = colsCountA;
	rowsInColBlock = rowsCountB;
	colsInColBlock = colsCountB / procCount;
	
	int** rowBlockA = alloc_2d_int(rowsInRowBlock, colsInRowBlock);
	int** colBlockB = alloc_2d_int(rowsInColBlock, colsInColBlock);
	int** rowBlockC = alloc_2d_int(rowsInRowBlock, colsCountB);

	int** tempSendCol = alloc_2d_int(rowsInColBlock, colsInColBlock);
	int** tempRecvCol = alloc_2d_int(rowsInColBlock, colsInColBlock);
	
	MPI_Datatype rowBlockType;
	MPI_Datatype localColType;
	MPI_Datatype colBlockType;
	// MPI_Request sendRowRequest = MPI_REQUEST_NULL;
	// MPI_Request recvRowRequest = MPI_REQUEST_NULL;
	// MPI_Status sendRowStatus;
	// MPI_Status recvRowStatus;
	MPI_Type_vector(rowsCountB, colsInColBlock, colsCountB, MPI_INT, &localColType); // creating column block type
	MPI_Type_create_resized(localColType, 0, sizeof(int) * colsInColBlock, &colBlockType);// creating column type
	MPI_Type_commit(&colBlockType);
	
	MPI_Type_vector(rowsInRowBlock, colsCountA, colsCountA, MPI_INT, &rowBlockType); // creating row type
	MPI_Type_commit(&rowBlockType);
	
	
	if (!myrank)
	{
		char* filename = argv[4];
		FILE* inputFile;
		int** matrA; 
		int** matrB; 
		matrA = alloc_2d_int(rowsCountA, colsCountA);
		matrB = alloc_2d_int(rowsCountB, colsCountB);
		
		inputFile = fopen(filename, "r");

		for (i = 0; i < rowsCountA; i++)
		{
			for (j = 0; j < colsCountA; j++)
			{
				fscanf(inputFile, "%d", &matrA[i][j]);
			}
		}
		
		for (i = 0; i < rowsCountB; i++)
		{
			for (j = 0; j < colsCountB; j++)
			{
				fscanf(inputFile, "%d", &matrB[i][j]);
			}
		}
		
		MPI_Scatter(matrA[0], 1, rowBlockType, *rowBlockA, elementsInRowBlockCount, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Scatter(matrB[0], 1, colBlockType, *colBlockB, elementsInColBlockCount, MPI_INT, 0, MPI_COMM_WORLD);
		
		free(matrA[0]);
		free(matrA);
		free(matrB[0]);
		free(matrB);
		fclose(inputFile);
	}
	else
	{
		MPI_Scatter(NULL, 1, rowBlockType, *rowBlockA, elementsInRowBlockCount, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Scatter(NULL, 1, colBlockType, *colBlockB, elementsInColBlockCount, MPI_INT, 0, MPI_COMM_WORLD);
	}	
	
	// // j = myrank;
	// // for (i = 0; i < n; i++)
	// // {
		// // int nextColNum;
		// // if (i)
		// // {
			// // //wait recv row
			// // MPI_Wait(&recvRowRequest, &recvRowStatus);
			// // copyArray(colBlockB, tempRecvRow, n);
		// // }
		
		// // nextColNum = j - 1;
		// // if (nextColNum < 0)
		// // {
			// // nextColNum += n;
		// // }
		
		// // if (i + 1 < n)
		// // {
			// // MPI_Irecv(tempRecvRow, 1, rowBlockType, leftRank, nextColNum, MPI_COMM_WORLD, &recvRowRequest);
		// // }
		
		// // rowBlockC[j] = 0;
		// // for (k = 0; k < n; k++)
		// // {
			// // rowBlockC[j] += rowBlockA[k] * colBlockB[k];
		// // }
		
		// // if (i)
		// // {
			// // MPI_Wait(&sendRowRequest, &sendRowStatus);
		// // }
		
		// // copyArray(tempSendRow, colBlockB, n);
		// // if (i + 1 < n)
		// // {
			// // MPI_Isend(tempSendRow, 1, rowBlockType, rightRank, j, MPI_COMM_WORLD, &sendRowRequest);
		// // }
		
		// // j = nextColNum;
	// // }
	// // if (!myrank)
	// // {
		// // int** matrC;
		// // FILE* output;
		// // matrC = alloc_2d_int(n, n);
		// // MPI_Gather(rowBlockC, n, MPI_INT, matrC[0], n, MPI_INT, 0, MPI_COMM_WORLD);
		// // output = fopen("output.txt", "w");
		// // for (i = 0; i < n; i++)
		// // {
			// // for (j = 0; j < n; j++)
			// // {
				// // fprintf(output, "%d ", matrC[i][j]);
			// // }
			
			// // fprintf(output, "\n");
		// // }
		// // free(matrC[0]);
		// // free(matrC);
	// // }
	// // if (myrank)
	// // {
		// // MPI_Gather(rowBlockC, n, MPI_INT, NULL, n, MPI_INT, 0, MPI_COMM_WORLD);
	// // }
	
	// MPI_Type_free(&rowBlockType);
	// MPI_Type_free(&colBlockType);
	// // free(tempRecvRow);
	// // free(tempSendRow);
	// // free(rowBlockA);
	// // free(colBlockB);
	// // free(rowBlockC);
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
