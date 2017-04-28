#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void My_Bcast(void* data, int count, MPI_Datatype datatype, int root, MPI_Comm comm);

int main(int argc, char** argv)
{
	int size;
	int myrank;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	int n = atoi(argv[1]);
	int r = atoi(argv[2]);
	
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	int* numberSeq = (int*)malloc(sizeof(int) * n);
	if (myrank == r)
	{
		int i;
		for (i = 0; i < n; i++)
			numberSeq[i] = rand() % 100;
	}
	My_Bcast(numberSeq, n, MPI_INT, r, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	printf("process %d\n", myrank);
	{
		int i;
		for (i = 0; i < n; i++)
			printf("%d ", numberSeq[i]);
		printf("\n");
	}
	MPI_Finalize();
	return 0;
}

void My_Bcast(void* data, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
	MPI_Comm bcastComm;
	MPI_Group oldGroup;
	MPI_Group bcastGroup;
	int* ranks;
	int i;
	int delta;
	int size;
	MPI_Comm_size(comm, &size);
	ranks = (int*)malloc(size * sizeof(int));
	for (i = 0; i < size; i++)
		ranks[i] = i;
	ranks[0] = root;
	ranks[root] = 0;
	MPI_Comm_group(comm, &oldGroup);
	MPI_Group_incl(oldGroup, size, ranks, &bcastGroup);
	MPI_Group_free(&oldGroup);
	MPI_Comm_create(comm, bcastGroup, &bcastComm);
	
	
	int world_rank;
	MPI_Comm_rank(bcastComm, &world_rank);
	int world_size;
	MPI_Comm_size(bcastComm, &world_size);

	if (!world_rank) 
	{
		delta = 1;
		for (i = world_rank + delta; i < world_size; i += delta, delta *= 2) 
		{
			printf("send from %d to %d\n", world_rank, i);
			MPI_Send(data, count, datatype, i, 10, bcastComm);
		}
	} 
	else 
	{
		MPI_Recv(data, count, datatype, MPI_ANY_SOURCE, 10, bcastComm, MPI_STATUS_IGNORE);
		delta = 1;
		while (delta <= world_rank)
			delta <<= 1;
		for (i = world_rank + delta; i < world_size; i += delta, delta *= 2) 
		{
			printf("send from %d to %d\n", world_rank, i);
			MPI_Send(data, count, datatype, i, 10, bcastComm);
		}
	}
	
	MPI_Group_free(&bcastGroup);
	MPI_Comm_free(&bcastComm);
}
