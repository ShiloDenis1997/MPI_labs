#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int **alloc_2d_int(int rows, int cols);
void copyBlock(int** destBlock, int** sourceBlock, int rows, int cols);
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
			printf("%d ", matr[i][j]);
		}
		printf("\n");
	}
}

void MultipleMatrixes(int** matrA, int** matrB, int** matrC, int rowsA, int colsA, int rowsB, int colsB)
{
	int rowsC = rowsA;
	int colsC = colsB;
	int i, j, q;
	for (i = 0; i < rowsC; i++)
	{
		for (j = 0; j < colsC; j++)
		{
			matrC[i][j] = 0;
		}
	}
	
	for (i = 0; i < rowsC; i++)
	{
		for (q = 0; q < colsA; q++)
		{
			for (j = 0; j < colsC; j++)
			{

				matrC[i][j] += matrA[i][q] * matrB[q][j];
			}
		}
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
	int leftProcessRank, rightProcessRank;
	int currentColBlockNumber;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	leftProcessRank = countLeftProcessRank(myrank, procCount);
	rightProcessRank = countRightProcessRank(myrank, procCount);
	
	rowsInRowBlock = rowsCountA / procCount;
	colsInRowBlock = colsCountA;
	rowsInColBlock = rowsCountB;
	colsInColBlock = colsCountB / procCount;
	
	int elementsInRowBlockCount = rowsInRowBlock * colsInRowBlock;
	int elementsInColBlockCount = rowsInColBlock * colsInColBlock;
	int elementsInRowBlockC = rowsInRowBlock * colsCountB;
	
	int** rowBlockA = alloc_2d_int(rowsInRowBlock, colsInRowBlock);
	int** colBlockB = alloc_2d_int(rowsInColBlock, colsInColBlock);
	int** rowBlockC = alloc_2d_int(rowsInRowBlock, colsCountB);

	int** tempSendCol = alloc_2d_int(rowsInColBlock, colsInColBlock);
	int** tempRecvCol = alloc_2d_int(rowsInColBlock, colsInColBlock);
	int** countedCRectangle = alloc_2d_int(rowsInRowBlock, colsInColBlock);
	
	MPI_Request sendColBlockRequest = MPI_REQUEST_NULL;
	MPI_Request recvColBlockRequest = MPI_REQUEST_NULL;
	MPI_Status sendRowStatus;
	MPI_Status recvRowStatus;
	
	MPI_Datatype rowBlockType;
	MPI_Datatype localColType;
	MPI_Datatype colBlockType;
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
	
	currentColBlockNumber = myrank;
	
	for (i = 0; i < procCount; i++)
	{
		int wantedColBlockNumber;
		if (i)
		{
			MPI_Wait(&recvColBlockRequest, &recvRowStatus);
			copyBlock(colBlockB, tempRecvCol, rowsInColBlock, colsInColBlock);
		}
		
		if (i + 1 < procCount)
		{
			if (i)
			{
				MPI_Wait(&sendColBlockRequest, &sendRowStatus);
			}
			copyBlock(tempSendCol, colBlockB, rowsInColBlock, colsInColBlock);
			MPI_Isend(*tempSendCol, elementsInColBlockCount, MPI_INT, rightProcessRank, currentColBlockNumber, MPI_COMM_WORLD, &sendColBlockRequest);
		}
		
		wantedColBlockNumber = countLeftProcessRank(currentColBlockNumber, procCount);
		
		if (i + 1 < procCount)
		{
			MPI_Irecv(*tempRecvCol, elementsInColBlockCount, MPI_INT, leftProcessRank, wantedColBlockNumber, MPI_COMM_WORLD, &recvColBlockRequest);
		}
		
		MultipleMatrixes(rowBlockA, colBlockB, countedCRectangle, rowsInRowBlock, colsInRowBlock, rowsInColBlock, colsInColBlock);
		{
			int i1, j1;
			for (i1 = 0; i1 < rowsInRowBlock; i1++)
			{
				for (j1 = 0; j1 < colsInColBlock; j1++)
				{
					rowBlockC[i1][j1 + currentColBlockNumber * colsInColBlock] = countedCRectangle[i1][j1];
				}
			}
		}
		
		currentColBlockNumber = wantedColBlockNumber;
	}
	
	if (!myrank)
	{
		int** matrC;
		FILE* output;
		matrC = alloc_2d_int(rowsCountA, colsCountB);
		MPI_Gather(*rowBlockC, elementsInRowBlockC, MPI_INT, *matrC, elementsInRowBlockC, MPI_INT, 0, MPI_COMM_WORLD);
		output = fopen("output.txt", "w");
		for (i = 0; i < rowsCountA; i++)
		{
			for (j = 0; j < colsCountB; j++)
			{
				fprintf(output, "%d ", matrC[i][j]);
			}
			
			fprintf(output, "\n");
		}
		free(matrC[0]);
		free(matrC);
	}
	else
	{
		MPI_Gather(*rowBlockC, elementsInRowBlockC, MPI_INT, NULL, elementsInRowBlockC, MPI_INT, 0, MPI_COMM_WORLD);
	}
	
	MPI_Type_free(&rowBlockType);
	MPI_Type_free(&colBlockType);
	free(tempRecvCol);
	free(tempSendCol);
	free(rowBlockA);
	free(colBlockB);
	free(rowBlockC);
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

void copyBlock(int** destBlock, int** sourceBlock, int rows, int cols)
{
	int i, j;
	for (i = 0; i < rows; i++)
		for (j = 0; j < cols; j++)
		{
			destBlock[i][j] = sourceBlock[i][j];
		}
}
