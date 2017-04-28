#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <unistd.h>

double CountPi(int n, int p, int procNumber)
{
	int i, start;
	double h, sum, x;
	double step = 1. / n;
	sum = 0.;
	start = procNumber;
	for (i = start; i<n; i += p)
	{
		x = step * i;
		sum += 4. / (1. + x*x);
	}
	return  step * sum;
}

int main(int argc, char**argv)
{
	int p = atoi(argv[1]);
	int n = atoi(argv[2]);
	pid_t chPID;
	int toSlaves[2];
	int fromSlaves[2];
	pipe(toSlaves);
	pipe(fromSlaves);

	struct sembuf operations[1];
	int semKey = 42;
	int semId = semget(semKey, 1, 0666 | IPC_CREAT);

	union semun {
		int val;
		struct semid_ds *buf;
		ushort * array;
	} argument;

   	argument.val = 0;
	if (semctl(semId, 0, SETVAL, argument) < 0)
	{
		fprintf(stderr, "Cannot set initial value of semaphore\n");
	}

	for (int i = 1; i < p; i++)
	{
		chPID = fork();
		if (!chPID)
		{
			break;
		}
	}

	operations[0].sem_num = 0;
	operations[0].sem_flg = 0;
	if (chPID == 0)
	{
		operations[0].sem_op = -1;
printf("Slave\n");
		close(toSlaves[1]);
		close(fromSlaves[0]);
		int procNumber;
		semop(semId, operations, 1);
		read(toSlaves[0], &procNumber, sizeof(int));
		operations[0].sem_op = 1;
		semop(semId, operations, 1);
		double res = CountPi(n, p, procNumber);
		write(fromSlaves[1], &res, sizeof(double));
	}
	else
	{
		operations[0].sem_op = 1;
		close(toSlaves[0]);
		close(fromSlaves[1]);
		for (int pNumber = 2; pNumber <= p; pNumber++)
		{
			write(toSlaves[1], &pNumber, sizeof(int));
		}
		semop(semId, operations, 1);
printf("Master\n");
		double res = CountPi(n, p, 1);
		double slRes;
		for (int pNumber = 2; pNumber <= p; pNumber++)
		{
			read(fromSlaves[0], &slRes, sizeof(double));
			res += slRes;
		}
		printf("%.5lf\n", res); 
	}

	return 0;
}

