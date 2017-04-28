#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	int size;
	int myrank;
	int tag = 10;
	int i;
	char str[100];
	char** resultBuffer;
	int number_count;
	MPI_Status status;

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &size);

	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	sprintf(str, "Hello from process %d", myrank);

	if (myrank)
	{
		MPI_Send(str, strlen(str) + 1, MPI_CHAR, 0, tag, MPI_COMM_WORLD);
	}
	else
	{
		resultBuffer = (char**)malloc(sizeof(char*) * size);
		resultBuffer[0] = (char*)malloc(sizeof(char) * (strlen(str) + 1));
		strcpy(resultBuffer[0], str);
		for (i = 1; i < size; i++)
		{
			MPI_Probe(MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &status);
			MPI_Get_count(&status, MPI_CHAR, &number_count);
			resultBuffer[status.MPI_SOURCE] = (char*)malloc(sizeof(char) * number_count);
			int source = status.MPI_SOURCE;
			MPI_Recv(resultBuffer[source], number_count, MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);
			printf("size: %d\n", number_count);
		}

		for (i = 0; i < size; i++)
		{
			printf("%s\n", resultBuffer[i]);
			free(resultBuffer[i]);
		}
		free(resultBuffer);
	}

	MPI_Finalize();
	return 0;
}
