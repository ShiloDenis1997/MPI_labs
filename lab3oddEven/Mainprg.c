#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int leftProcessRank, rightProcessRank;
int leftProcessBufferCount, rightProcessBufferCount;

int* alloc_int_array(int numberOfElements)
{
	return (int* )malloc(numberOfElements*sizeof(int));
}

int countLeftProcessRank(int myrank, int procCount)
{
	return myrank - 1;
}

int countRightProcessRank(int myrank, int procCount)
{
	return myrank + 1;
}

int getRankOfProcessToCommunicate(int myrank, int level)
{
	if (level & 1)
	{
		if (myrank & 1)
		{
			return rightProcessRank;
		}
		else
		{
			return leftProcessRank;
		}
	}
	else
	{
		if (myrank & 1)
		{
			return leftProcessRank;
		}
		else
		{
			return rightProcessRank;
		}
	}
}

int getBufferSizeByRank(int myrank, int countOfElements, int procCount)
{
	if (myrank < 0 || myrank >= procCount)
	{
		return 0;
	}
	
	int bufferSize = countOfElements / procCount;
	if (myrank < countOfElements % procCount)
	{
		bufferSize++;
	}
	
	return bufferSize;
}

void localSort(int* array, int elementsCount)
{
	int i, j;
	for (i = 0; i < elementsCount; i++)
	{
		for (j = i + 1; j < elementsCount; j++)
		{
			if (array[i] > array[j])
			{
				int t = array[i];
				array[i] = array[j];
				array[j] = t;
			}
		}
	}
}

int isTakePartInComputations(int myrank, int procCount, int level)
{
	if (procCount == 1)
	{	
		return 0;
	}
	
	if (myrank > 0 && myrank < procCount - 1)
	{
		return 1;
	}
	
	if (level & 1)
	{
		if (myrank == 0)
		{
			return 0;
		}
		if ((myrank & 1) && (myrank + 1 == procCount))
		{
			return 0;
		}
	}
	else
	{
		if (!(myrank&1) && myrank + 1 == procCount)
		{
			return 0;
		}
	}
	
	return 1;
}

void mergeArrays(int* mergedBuffer, int* firstArray, int* secondArray, int firstArraySize, int secondArraySize)
{
	int resultSize = firstArraySize + secondArraySize;
	int firstPos = 0, secondPos = 0, i;
	for (i = 0; i < resultSize; i++)
	{
		if (firstPos >= firstArraySize)
		{
			mergedBuffer[i] = secondArray[secondPos++];
		}
		else if (secondPos >= secondArraySize)
		{
			mergedBuffer[i] = firstArray[firstPos++];
		}
		else if (firstArray[firstPos] < secondArray[secondPos])
		{
			mergedBuffer[i] = firstArray[firstPos++];
		}
		else
		{
			mergedBuffer[i] = secondArray[secondPos++];
		}
	}
}

void remainArrayPart(int* mergedPart, int sendRecvProcRank, int* myArrayPart, int myBufferSize, int sendRecvProcBufferSize)
{
	int i, delta = 0;
	if (sendRecvProcRank == leftProcessRank)
	{
		delta = sendRecvProcBufferSize;
	}
	
	for (i = 0; i < myBufferSize; i++)
	{
		myArrayPart[i] = mergedPart[delta + i];
	}
}

int main(int argc, char** argv)
{
	int myrank;
	int i, j, k;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	int elementsCount = atoi(argv[1]);
	int procCount = atoi(argv[2]);
	
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	leftProcessRank = countLeftProcessRank(myrank, procCount);
	rightProcessRank = countRightProcessRank(myrank, procCount);
	leftProcessBufferCount = getBufferSizeByRank(leftProcessRank, elementsCount, procCount);
	rightProcessBufferCount = getBufferSizeByRank(rightProcessRank, elementsCount, procCount);

	MPI_Status sendRecvStatus;
	
	int myBufferSize = getBufferSizeByRank(myrank, elementsCount, procCount);
	int* myArrayPart = alloc_int_array(myBufferSize);
	int* recvBuffer = alloc_int_array(myBufferSize + 1);
	int* mergedPart = alloc_int_array(myBufferSize * 2 + 1);
	int sendRecvProcRank;
	int sendRecvProcBufferSize;
	
	int* arrayToSort;
	int* bufferSizes;
	int* displs;
	
	if (!myrank)
	{
		char* filename = argv[3];
		FILE* inputFile;
		arrayToSort = alloc_int_array(elementsCount);
		bufferSizes = alloc_int_array(procCount);
		displs = alloc_int_array(procCount);
		
		inputFile = fopen(filename, "r");

		for (i = 0; i < elementsCount; i++)
		{
			fscanf(inputFile, "%d", &arrayToSort[i]);
		}
		
		for (i = 0; i < procCount; i++)
		{
			bufferSizes[i] = getBufferSizeByRank(i, elementsCount, procCount);
			if (i != 0)
			{
				displs[i] = displs[i - 1] + bufferSizes[i - 1];
			}
			else
			{
				displs[i] = 0;
			}
		}
		
		MPI_Scatterv(arrayToSort, bufferSizes, displs, MPI_INT, myArrayPart, myBufferSize, MPI_INT, 0, MPI_COMM_WORLD);
		
		fclose(inputFile);
	}
	else
	{
		MPI_Scatterv(NULL, NULL, NULL, MPI_INT, myArrayPart, myBufferSize, MPI_INT, 0, MPI_COMM_WORLD);
	}	

	localSort(myArrayPart, myBufferSize);
	
	for (i = 0; i < procCount * 2; i++)
	{
		if (!isTakePartInComputations(myrank, procCount, i))
		{
			continue;
		}
		
		sendRecvProcRank = getRankOfProcessToCommunicate(myrank, i);
		sendRecvProcBufferSize = getBufferSizeByRank(sendRecvProcRank, elementsCount, procCount);
		
		MPI_Sendrecv(myArrayPart, myBufferSize, MPI_INT, sendRecvProcRank, 10,
					recvBuffer, sendRecvProcBufferSize, MPI_INT, sendRecvProcRank, 10, MPI_COMM_WORLD, &sendRecvStatus);
		
		mergeArrays(mergedPart, myArrayPart, recvBuffer, myBufferSize, sendRecvProcBufferSize);
		remainArrayPart(mergedPart, sendRecvProcRank, myArrayPart, myBufferSize, sendRecvProcBufferSize);
	}
	
	// for (i = 0; i < myBufferSize; i++)
	// {
		// printf("%d ", myArrayPart[i]);
	// }
	// printf("\n");
	
	 if (!myrank)
	 {
		int** matrC;
		FILE* output;
		MPI_Gatherv(myArrayPart, myBufferSize, MPI_INT, arrayToSort, bufferSizes, displs, MPI_INT, 0, MPI_COMM_WORLD);
		output = fopen("output.txt", "w");
		for (i = 0; i < elementsCount; i++)
		{
			fprintf(output, "%d ", arrayToSort[i]);
		}
		
		free(arrayToSort);
		free(bufferSizes);
		free(displs);
	 }
	 else
	 {
		MPI_Gatherv(myArrayPart, myBufferSize, MPI_INT, NULL, NULL, NULL, MPI_INT, 0, MPI_COMM_WORLD);
	 }
	
	free(myArrayPart);
	free(recvBuffer);
	free(mergedPart);
	MPI_Finalize();
	return 0;
}
