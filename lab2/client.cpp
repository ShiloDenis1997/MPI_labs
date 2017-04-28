#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <stdio.h>
#include <unistd.h>

#define SHMSZ 255
#define SHMKEY 5679
#define SEMWKEY 42
#define SEMRKEY 56

int InitSemaphore(int semKey)
{
	int id = semget(semKey, 1, 0666 | IPC_CREAT);
	return id;
}

int main()
{
    char c;
    int shmid;
    key_t key;
    char *shm, *s;
	struct sembuf waitOperation[1];
	struct sembuf releaseOperation[1];
	
	waitOperation[0].sem_num = 0;
	waitOperation[0].sem_flg = 0;
	waitOperation[0].sem_op = -1;
	releaseOperation[0].sem_num = 0;
	releaseOperation[0].sem_flg = 0;
	releaseOperation[0].sem_op = 1;
	
    /*
     * Create the segment.
     */
    if ((shmid = shmget(SHMKEY, SHMSZ, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        return 1;
    }

    /*
     * Now we attach the segment to our data space.
     */
    if ((shm = (char*)shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("shmat");
        return 1;
    }

    int semWriteId = InitSemaphore(SEMWKEY);
	int semReadId = InitSemaphore(SEMRKEY);
	
	char *mes = (char*)shm;
	char userInput[SHMSZ];
	
	while (true)
	{
		printf("Enter message:");
		scanf("%s", userInput);
		printf("Sending...\n");
		semop(semWriteId, waitOperation, 1);
		sprintf(mes, "%s", userInput);
		printf("Sended.\n");
		semop(semReadId, releaseOperation, 1);
	}

    return 0;
}
