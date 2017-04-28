#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//void MatrixMultiplicationMPI(double *&A, double *&B, double *&C, int &Size);
int **alloc_2d_int(int rows, int cols);

int main(int argc, char** argv)
{
	int size;
	int myrank;
	int i, j;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	int n = atoi(argv[1]); // size of matrix
	int p = atoi(argv[2]); //number of processes
	
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	int* rowA = (int*)malloc(sizeof(int) * n);
	int* rowB = (int*)malloc(sizeof(int) * n);
	int* rowC = (int*)malloc(sizeof(int) * n);
	
	
	if (!myrank)
	{
		char* filename = argv[3];
		FILE* inputFile;
		int** matrA; 
		int** matrB; 
		int** matrC; 
		matrA = alloc_2d_int(n, n);
		matrB = alloc_2d_int(n, n);
		matrC = alloc_2d_int(n, n);
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
		
		
		free(matrA[0]);
		free(matrA);
		free(matrB[0]);
		free(matrB);
		free(matrC[0]);
		free(matrC);
		fclose(inputFile);
	}
		
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

// void MatrixMultiplicationMPI(double *&A, double *&B, double *&C, int &Size) 
// {
	// int dim = Size;
	// int i, j, k, p, ind;
	// double temp;
	// MPI_Status Status;
	// int ProcPartSize = dim/ProcNum; 
	// int ProcPartElem = ProcPartSize*dim; 
	// double* bufA = new double[ProcPartElem];
	// double* bufB = new double[ProcPartElem];
	// double* bufC = new double[ProcPartElem];
	// int ProcPart = dim/ProcNum, part = ProcPart*dim;
	// if (ProcRank == 0) {
		// Flip(B, Size);
	// }
	
	// MPI_Scatter(A, part, MPI_DOUBLE, bufA, part, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	// MPI_Scatter(B, part, MPI_DOUBLE, bufB, part, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	
	// temp = 0.0;
	// for (i=0; i < ProcPartSize; i++) {
		// for (j=0; j < ProcPartSize; j++) {
			// for (k=0; k < dim; k++) 
				// temp += bufA[i*dim+k]*bufB[j*dim+k];
			// bufC[i*dim+j+ProcPartSize*ProcRank] = temp;
			// temp = 0.0;
		// }
	// }

	// int NextProc; int PrevProc;
	// for (p=1; p < ProcNum; p++) {
		// NextProc = ProcRank+1;
		// if (ProcRank == ProcNum-1) 
			// NextProc = 0;
		// PrevProc = ProcRank-1;
		// if (ProcRank == 0) 
			// PrevProc = ProcNum-1;
		// MPI_Sendrecv_replace(bufB, part, MPI_DOUBLE, NextProc, 0, PrevProc, 0, MPI_COMM_WORLD, &Status);
		// temp = 0.0;
		// for (i=0; i < ProcPartSize; i++) {
			// for (j=0; j < ProcPartSize; j++) {
				// for (k=0; k < dim; k++) {
					// temp += bufA[i*dim+k]*bufB[j*dim+k];
				// }
				// if (ProcRank-p >= 0 ) 
					// ind = ProcRank-p;
				// else ind = (ProcNum-p+ProcRank);
				// bufC[i*dim+j+ind*ProcPartSize] = temp;
				// temp = 0.0;
			// }
		// }
	// }
	
	// MPI_Gather(bufC, ProcPartElem, MPI_DOUBLE, C, ProcPartElem, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	// delete []bufA;
	// delete []bufB;
	// delete []bufC;
// }
