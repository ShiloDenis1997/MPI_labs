#include <omp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int procCount;
int optimalLineHeight;
int matrixDimSize;
int linesCount;
 
double* alloc_double_array(int numberOfElements)
{
	return (double* )malloc(numberOfElements*sizeof(double));
}

void readTask(double* a, double* b, double* c, double* f, char* filename, int matrixDimSize)
{
	int i;
	FILE* inputFile = fopen(filename, "r");

	for (i = 0; i < matrixDimSize - 1; i++)
	{
		fscanf(inputFile, "%lf", &b[i]);
	}
	b[matrixDimSize - 1] = 0;
	for (i = 0; i < matrixDimSize; i++)
	{
		fscanf(inputFile, "%lf", &c[i]);
	}
	a[0] = 0;
	for (i = 1; i < matrixDimSize; i++)
	{
		fscanf(inputFile, "%lf", &a[i]);
	}
	
	for (i = 0; i < matrixDimSize; i++)
	{
		fscanf(inputFile, "%lf", &f[i]);
	}
	
	fclose(inputFile);
}

int countOptimalLineHeight()
{
	int optimalLineHeight = matrixDimSize / linesCount;
	if (optimalLineHeight * linesCount != matrixDimSize)
	{
		optimalLineHeight++;
	}
	return optimalLineHeight;
}

void countBoundaries(int myLineNumber, int* top, int* bottom)
{
	*top = myLineNumber * optimalLineHeight;
	*bottom = *top + optimalLineHeight;
	if (*bottom > matrixDimSize)
	{
		*bottom = matrixDimSize;
	}
}

void straightMove(double* a, double* b, double* c, double* f, int myLineNumber)
{
	int top, bottom, i;
	double multipler;
	countBoundaries(myLineNumber, &top, &bottom);
	
	for (i = top; i < bottom - 1; i++)
	{
		multipler = a[i + 1] / c[i];
		a[i + 1] = -multipler * a[i];
		c[i + 1] -= multipler * b[i];
		f[i + 1] -= multipler * f[i];
	}
}

void reverseMove(double* a, double* b, double* c, double* f, int myLineNumber)
{
	int top, bottom, i;
	double multipler;
	countBoundaries(myLineNumber, &top, &bottom);
	
	for (i = bottom - 2; i >= top; i--)
	{
		if (i != 0)
		{
			multipler = b[i - 1] / c[i];
			b[i - 1] = -multipler * b[i];
			f[i - 1] -= multipler * f[i];
			if (i != top)
			{
				a[i - 1] -= multipler * a[i];
			}
			else
			{
				c[i - 1] -= multipler * a[i];
			}
		}
	}
}

int countNextIndex(int currentIndex)
{
	int nextIndex = currentIndex + optimalLineHeight;
	if (nextIndex >= matrixDimSize)
	{
		nextIndex = matrixDimSize - 1;
	}
	return nextIndex;
}

void cycle_reduction(double* a, double* b, double* c, double* f, double* x)
{
	double multipler;
	int i, currentIndex, nextIndex, predIndex;
	//straight move
	currentIndex = optimalLineHeight - 1;
	predIndex = currentIndex;
	while (1)
	{
		nextIndex = countNextIndex(currentIndex);
		if (nextIndex == currentIndex)
		{
			break;
		}
		multipler = a[nextIndex] / c[currentIndex];
		a[nextIndex] = 0;
		c[nextIndex] -= multipler * b[currentIndex];
		f[nextIndex] -= multipler * f[currentIndex];
		predIndex = currentIndex;
		currentIndex = countNextIndex(currentIndex);
	}
	
	//reverse move
	x[currentIndex] = f[currentIndex] / c[currentIndex];
	nextIndex = currentIndex;
	currentIndex = predIndex;
	while (currentIndex > 0)
	{
		x[currentIndex] = (f[currentIndex] - x[nextIndex] * b[currentIndex]) / c[currentIndex];
		nextIndex = currentIndex;
		currentIndex -= optimalLineHeight;
	}
}

void countResult(double* a, double* b, double* c, double* f, double* x, int myLineNumber)
{
	int top, bottom, i, x1Index, x2Index;
	double multipler;
	countBoundaries(myLineNumber, &top, &bottom);
	x1Index = top - 1;
	x2Index = bottom - 1;
	
	for (i = bottom - 2; i >= top; i--)
	{
		x[i] = f[i] - x[x2Index] * b[i];
		if (x1Index > 0)
		{
			x[i] -= x[x1Index] * a[i];
		}
		x[i] /= c[i];
	}
}

void solveTask(double* a, double* b, double* c, double* f, double* x)
{
	int i;
	#pragma omp parallel for
	for (i = 0; i < linesCount; i++)
	{
		straightMove(a, b, c, f, i);
	}
	
	#pragma omp parallel for
	for (i = 0; i < linesCount; i++)
	{
		reverseMove(a, b, c, f, i);
	}
	
	//cycle reduction (sequence part)
	// printf("Thread rank: %d\n", omp_get_num_threads());
	cycle_reduction(a, b, c, f, x);
	
	#pragma omp parallel for
	for (i = 0; i < linesCount; i++)
	{
		countResult(a, b, c, f, x, i);
	}
}

int main(int argc, char** argv)
{
	int i, j, k;
	matrixDimSize = atoi(argv[1]);
	procCount = atoi(argv[2]);
	linesCount = procCount;
	optimalLineHeight = countOptimalLineHeight();
	
	double* b = alloc_double_array(matrixDimSize);
	double* c = alloc_double_array(matrixDimSize);
	double* a = alloc_double_array(matrixDimSize);
	double* f = alloc_double_array(matrixDimSize);
	double* x = alloc_double_array(matrixDimSize);
	char* filename = argv[3];
	
	readTask(a, b, c, f, filename, matrixDimSize);
	
	omp_set_num_threads(procCount);
	solveTask(a, b, c, f, x);
	
	for (i = 0; i < matrixDimSize; i++)
	{
		printf("%.2lf \t%.2lf \t%.2lf \t%.2lf \t%.2lf\n", a[i], c[i], b[i], f[i], x[i]);
	}
	
	return 0;
}