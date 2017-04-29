#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

int main(int argc, char** argv)
{
	int procNum = atoi(argv[1]);
	double sum = 0;
	int n = atoi(argv[2]);
	int i;
	double	x;
	double step = 1.0 / n;
	omp_set_num_threads(procNum);

	#pragma omp parallel for reduction(+:sum) private(x)
	for (i=1; i<n; i++)
	{
		x = step * i;
		sum += 4.0 / (1.0 + x*x) * step;
	}

	printf("%.4lf\n", sum);
	return 0;
}
