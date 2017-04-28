#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

int n, p;
int partSize;
int curPos = 0;
pthread_mutex_t mutexGetPart = PTHREAD_MUTEX_INITIALIZER;

void* CountPi(void *answer)
{
	int i, start;
	double h, sum, x;
	double step = 1. / n;
	sum = 0.;
	while (true)
	{
		pthread_mutex_lock(&mutexGetPart);
		start = curPos;
		curPos += partSize;
		pthread_mutex_unlock(&mutexGetPart);
		if (start >= n)
		{
			break;
		}

		int end = start + partSize;
		if (end > n)
		{
			end = n;
		}

		for (i = start; i<end; i++)
		{
			x = step * i;
			sum += 4. / (1. + x*x);
		}
	}
	double res = step*sum;
	*((double *)(answer)) = res;
	return  0;
}

int main(int argc, char**argv)
{
	p = atoi(argv[1]);
	n = atoi(argv[2]);
	partSize = atoi(argv[3]);
	int rc;
	pthread_t *threads = new pthread_t[p];
	double *answers = new double[p];

	for (int i = 1; i < p; i++)
	{
		rc = pthread_create(&threads[i], NULL, CountPi, (void*)&answers[i]);
		if (rc)
		{
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

	CountPi((void*)&answers[0]);
	for (int i = 1; i < p; i++)
	{
		pthread_join(threads[i], NULL);
	}

	double sum = 0;
	for (int i = 0; i < p; i++)
		sum += answers[i];
	printf("%.6lf\n", sum);

	pthread_mutex_destroy(&mutexGetPart);
	delete[] threads;
	delete[] answers;
	return 0;
}

