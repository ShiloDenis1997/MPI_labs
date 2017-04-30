#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int ndims = 2;

int **alloc_2d_int(int rows, int cols);
int* alloc_int_array(int numberOfElements);
void copyBlock(int** destBlock, int** sourceBlock, int rows, int cols);
int countLeftProcessRank(int myrank, int procCount)
{
	return (myrank + procCount - 1) % procCount;
}

int countRightProcessRank(int myrank, int procCount)
{
	return (myrank + 1) % procCount;
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
		for (q = 0; q < colsA; q++)
		{
			for (j = 0; j < colsC; j++)
			{

				matrC[i][j] += matrA[i][q] * matrB[q][j];
			}
		}
	}	
}

void createTopology(MPI_Comm *cartComm, MPI_Comm *rowComm, MPI_Comm *colComm, int sqrtProcCount)
{
	int dims[2] = {sqrtProcCount, sqrtProcCount};
	int periods[2] = {1, 0};
	int subdims[2];
	int reorder = 0;

	MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, periods, reorder, cartComm);
	subdims[0] = 0;
	subdims[1] = 1;
	MPI_Cart_sub(*cartComm, subdims, rowComm);
	subdims[0] = 1;
	subdims[1] = 0;
	MPI_Cart_sub(*cartComm, subdims, colComm);
	
	// int size;
	// MPI_Comm_size(*rowComm, &size);
	// printf("splised size = %d\n", size);
	// MPI_Barrier(MPI_COMM_WORLD);
}

int getGroups(MPI_Comm cartComm, MPI_Comm rowComm, MPI_Group* cartGroup, MPI_Group* rowGroup)
{
	MPI_Comm_group(cartComm, cartGroup);
	MPI_Comm_group(rowComm, rowGroup);		
}

int main(int argc, char** argv)
{
	int myrank;
	int i, j, k;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	int ATotalRows = atoi(argv[1]); // count of rows in A
	int ATotalCols = atoi(argv[2]); // count of columns in A
	int BTotalRows = ATotalCols; // count of rows in B
	int BTotalCols = ATotalRows; // count of columns in B
	int CTotalRows = ATotalRows;
	int CTotalCols = BTotalCols;
	int sqrtProcCount = atoi(argv[3]); // sqrt from count of processes
	int procCount = sqrtProcCount * sqrtProcCount; // count of processes
	int mainCoord[2];
	int coord[2];
	int wantedCoords[2];
	int mainCartRank;
	
	MPI_Comm cartComm;
	MPI_Comm rowComm;
	MPI_Comm colComm;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
		
	createTopology(&cartComm, &rowComm, &colComm, sqrtProcCount);
	

	// int rowsInRowBlock, rowsInColBlock, colsInRowBlock, colsInColBlock;
	// int leftProcessRank, rightProcessRank;
	// int currentColBlockNumber;
	
	int ARowsInBlock, AColsInBlock, BRowsInBlock, BColsInBlock, CRowsInBlock, CColsInBlock;
	
	ARowsInBlock = ATotalRows / sqrtProcCount;
	AColsInBlock = ATotalCols / sqrtProcCount;
	BRowsInBlock = BTotalRows / sqrtProcCount;
	BColsInBlock = BTotalCols / sqrtProcCount;
	CRowsInBlock = CTotalRows / sqrtProcCount;
	CColsInBlock = CTotalCols / sqrtProcCount;
	
	int ABlockElementsCount = ARowsInBlock * AColsInBlock;
	int BBlockElementsCount = BRowsInBlock * BColsInBlock;
	int CBlockElementsCount = CRowsInBlock * CColsInBlock;
	
	// leftProcessRank = countLeftProcessRank(myrank, sqrtProcCount);
	// rightProcessRank = countRightProcessRank(myrank, sqrtProcCount);
	
	int** aBlock = alloc_2d_int(ARowsInBlock, AColsInBlock);
	int** bBlock = alloc_2d_int(BRowsInBlock, BColsInBlock);
	int** cBlock = alloc_2d_int(CRowsInBlock, CColsInBlock);
	int** aBlockBuffer = alloc_2d_int(ARowsInBlock, AColsInBlock);
	int** bBlockBuffer = alloc_2d_int(BRowsInBlock, BColsInBlock);
	int* aSendCounts = alloc_int_array(procCount);
	int* aDispls = alloc_int_array(procCount);
	int* bSendCounts = alloc_int_array(procCount);
	int* bDispls = alloc_int_array(procCount);	
	int* cSendCounts = alloc_int_array(procCount);
	int* cDispls = alloc_int_array(procCount);
	
	int* cartRanks = alloc_int_array(procCount);
	int* rowRanks = alloc_int_array(procCount);
	for (i = 0; i < procCount; i++)
	{
		cartRanks[i] = i;
		rowRanks[i] = MPI_UNDEFINED;
	}
	
	for (i = 0; i < CRowsInBlock; i++)
	{
		for (j = 0; j < CColsInBlock; j++)
		{
			cBlock[i][j] = 0;
		}
	}
	
	for (i = 0; i < procCount; i++)
	{
		aSendCounts[i] = bSendCounts[i] = cSendCounts[i] = 1;
	}
	k = 0;
	aDispls[0] = bDispls[0] = cDispls[0] = 0;
	for (i = 0; i < sqrtProcCount; i++)
	{
		for (j = 0; j < sqrtProcCount; j++)
		{
			if (j != 0)
			{
				k++;
				aDispls[k] = aDispls[k - 1] + 1;
				bDispls[k] = bDispls[k - 1] + 1;
				cDispls[k] = cDispls[k - 1] + 1;
			}
		}
		if (k + 1 < procCount)
		{
			k++;
			aDispls[k] = aDispls[k - 1] + 1 + (ARowsInBlock - 1) * sqrtProcCount;
			bDispls[k] = bDispls[k - 1] + 1 + (BRowsInBlock - 1) * sqrtProcCount;
			cDispls[k] = cDispls[k - 1] + 1 + (CRowsInBlock - 1) * sqrtProcCount;
		}
	}
	
	// int** tempSendCol = alloc_2d_int(rowsInColBlock, colsInColBlock);
	// int** tempRecvCol = alloc_2d_int(rowsInColBlock, colsInColBlock);
	// int** countedCRectangle = alloc_2d_int(rowsInRowBlock, colsInColBlock);
	
	// MPI_Request sendColBlockRequest = MPI_REQUEST_NULL;
	// MPI_Request recvColBlockRequest = MPI_REQUEST_NULL;
	// MPI_Status sendRowStatus;
	// MPI_Status recvRowStatus;
	
	MPI_Datatype aBlockType;
	MPI_Datatype aLocalBlock;
	MPI_Datatype bBlockType;
	MPI_Datatype bLocalBlock;
	MPI_Datatype cBlockType;
	MPI_Datatype cLocalBlock;
	
	MPI_Type_vector(ARowsInBlock, AColsInBlock, ATotalCols, MPI_INT, &aLocalBlock);
	MPI_Type_create_resized(aLocalBlock, 0, sizeof(int) * AColsInBlock, &aBlockType);
	MPI_Type_commit(&aBlockType);
	
	MPI_Type_vector(BRowsInBlock, BColsInBlock, BTotalCols, MPI_INT, &bLocalBlock);
	MPI_Type_create_resized(bLocalBlock, 0, sizeof(int) * BColsInBlock, &bBlockType);
	MPI_Type_commit(&bBlockType);
	
	MPI_Type_vector(CRowsInBlock, CColsInBlock, CTotalCols, MPI_INT, &cLocalBlock);
	MPI_Type_create_resized(cLocalBlock, 0, sizeof(int) * CColsInBlock, &cBlockType);
	MPI_Type_commit(&cBlockType);
	
	MPI_Cart_coords(cartComm, 0, ndims, mainCoord);
	MPI_Cart_rank(cartComm, mainCoord, &mainCartRank);
	
	if (!myrank)
	{
		char* filename = argv[4];
		FILE* inputFile;
		int** matrA; 
		int** matrB; 
		matrA = alloc_2d_int(ATotalRows, ATotalCols);
		matrB = alloc_2d_int(BTotalRows, BTotalCols);
		
		inputFile = fopen(filename, "r");

		for (i = 0; i < ATotalRows; i++)
		{
			for (j = 0; j < ATotalCols; j++)
			{
				fscanf(inputFile, "%d", &matrA[i][j]);
			}
		}
		
		for (i = 0; i < BTotalRows; i++)
		{
			for (j = 0; j < BTotalCols; j++)
			{
				fscanf(inputFile, "%d", &matrB[i][j]);
			}
		}
		
		MPI_Scatterv(matrA[0], aSendCounts, aDispls, aBlockType, *aBlock, ABlockElementsCount, MPI_INT, mainCartRank, MPI_COMM_WORLD);
		MPI_Scatterv(matrB[0], bSendCounts, bDispls, bBlockType, *bBlock, BBlockElementsCount, MPI_INT, mainCartRank, MPI_COMM_WORLD);
		
		free(matrA[0]);
		free(matrA);
		free(matrB[0]);
		free(matrB);
		fclose(inputFile);
	}
	else
	{
		MPI_Scatterv(NULL, NULL, NULL, NULL, *aBlock, ABlockElementsCount, MPI_INT, mainCartRank, MPI_COMM_WORLD);
		MPI_Scatterv(NULL, NULL, NULL, NULL, *bBlock, BBlockElementsCount, MPI_INT, mainCartRank, MPI_COMM_WORLD);
	}	
	
	MPI_Cart_coords(cartComm, myrank, ndims, coord);
	wantedCoords[0] = coord[0];
	
	int myCartRank;
	MPI_Comm_rank(cartComm, &myCartRank);
	
	MPI_Group cartGroup;
	MPI_Group rowGroup;
	getGroups(cartComm, rowComm, &cartGroup, &rowGroup);
	
	MPI_Group_translate_ranks(cartGroup, procCount, cartRanks, rowGroup, rowRanks);
	MPI_Status bBlockRecvStatus;
	
	for (i = 0; i < sqrtProcCount; i++)
	{
		int wantedColBlockNumber = (coord[0] + i) % sqrtProcCount;
		wantedCoords[1] = wantedColBlockNumber;
		int wantedCartRank;
		int wantedRowRank;
		MPI_Cart_rank(cartComm, wantedCoords, &wantedCartRank);
		wantedRowRank = rowRanks[wantedCartRank];
		
		if (wantedCartRank == myCartRank)
		{
			copyBlock(aBlockBuffer, aBlock, ARowsInBlock, AColsInBlock);
		}
		
		MPI_Bcast(*aBlockBuffer, ABlockElementsCount, MPI_INT, wantedRowRank, rowComm);
		
		MultipleMatrixes(aBlockBuffer, bBlock, cBlock, ARowsInBlock, AColsInBlock, BRowsInBlock, BColsInBlock);
		
		copyBlock(bBlockBuffer, bBlock, BRowsInBlock, BColsInBlock);
		int fromRank, toRank;
		MPI_Cart_shift(cartComm, 0, -1, &fromRank, &toRank);
		
		if (coord[0] == 0)
		{
			MPI_Recv(*bBlock, BBlockElementsCount, MPI_INT, fromRank, 10, cartComm, &bBlockRecvStatus);
			MPI_Send(*bBlockBuffer, BBlockElementsCount, MPI_INT, toRank, 10, cartComm);
		}
		else
		{
			MPI_Send(*bBlockBuffer, BBlockElementsCount, MPI_INT, toRank, 10, cartComm);
			MPI_Recv(*bBlock, BBlockElementsCount, MPI_INT, fromRank, 10, cartComm, &bBlockRecvStatus);
		}
		// if (wantedCoords[1] == coord[1]) //i need to send
		// {
		//		int rankInRowComm = getRankInRowComm(wantedCoords, cartComm, rowComm);
		// }
		// else
		// {
			// //i recv block
		// }
		
		// int wantedColBlockNumber;
		// if (i)
		// {
			// MPI_Wait(&recvColBlockRequest, &recvRowStatus);
			// copyBlock(colBlockB, tempRecvCol, rowsInColBlock, colsInColBlock);
		// }
		
		
		// if (i + 1 < sqrtProcCount)
		// {
			// if (i)
			// {
				// MPI_Wait(&sendColBlockRequest, &sendRowStatus);
			// }
			// copyBlock(tempSendCol, colBlockB, rowsInColBlock, colsInColBlock);
			// MPI_Isend(*tempSendCol, elementsInColBlockCount, MPI_INT, rightProcessRank, currentColBlockNumber, MPI_COMM_WORLD, &sendColBlockRequest);
		// }
		
		// wantedColBlockNumber = countLeftProcessRank(currentColBlockNumber, sqrtProcCount);
		
		// if (i + 1 < sqrtProcCount)
		// {
			// MPI_Irecv(*tempRecvCol, elementsInColBlockCount, MPI_INT, leftProcessRank, wantedColBlockNumber, MPI_COMM_WORLD, &recvColBlockRequest);
		// }
		
		// MultipleMatrixes(rowBlockA, colBlockB, countedCRectangle, rowsInRowBlock, colsInRowBlock, rowsInColBlock, colsInColBlock);
		// {
			// int i1, j1;
			// for (i1 = 0; i1 < rowsInRowBlock; i1++)
			// {
				// for (j1 = 0; j1 < colsInColBlock; j1++)
				// {
					// rowBlockC[i1][j1 + currentColBlockNumber * colsInColBlock] = countedCRectangle[i1][j1];
				// }
			// }
		// }
		
		// currentColBlockNumber = wantedColBlockNumber;
	}
	
			if (coord[0] == 0 && coord[1] == 0){
			printMatrix(aBlockBuffer, ARowsInBlock, AColsInBlock);
			printMatrix(bBlock, BRowsInBlock, BColsInBlock);
			printMatrix(cBlock, CRowsInBlock, CColsInBlock);
		}
	
	if (!myrank)
	{
		int** matrC;
		FILE* output;
		matrC = alloc_2d_int(CTotalRows, CTotalCols);
		MPI_Gatherv(*cBlock, CBlockElementsCount, MPI_INT, *matrC, cSendCounts, cDispls, cBlockType, 0, cartComm);
		output = fopen("output.txt", "w");
		for (i = 0; i < CTotalRows; i++)
		{
			for (j = 0; j < CTotalCols; j++)
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
		MPI_Gatherv(*cBlock, CBlockElementsCount, MPI_INT, NULL, NULL, NULL, NULL, mainCartRank, cartComm);
	}
	
	// free(tempRecvCol);
	// free(tempSendCol);
	// free(rowBlockA);
	// free(colBlockB);
	// free(rowBlockC);
	MPI_Group_free(&cartGroup);
	MPI_Group_free(&rowGroup);
	MPI_Type_free(&aBlockType);
	MPI_Type_free(&bBlockType);
	MPI_Type_free(&cBlockType);
	MPI_Comm_free(&rowComm);
	MPI_Comm_free(&colComm);
	MPI_Comm_free(&cartComm);
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

int* alloc_int_array(int numberOfElements)
{
	return (int* )malloc(numberOfElements*sizeof(int));
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
