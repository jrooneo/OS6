/************************************************
*	$Author: o-rooneo $
*	$Date: 2015/04/30 09:00:43 $
*	$Log: userProcess.c,v $
*	Revision 1.4  2015/04/30 09:00:43  o-rooneo
*	Final commit. Project as complete as will be. Info in readme
*
*	Revision 1.3  2015/04/30 04:58:38  o-rooneo
*	Shares memory but not much more
*
*	Revision 1.2  2015/04/23 00:26:29  o-rooneo
*	added RCS header keywords
*
************************************************/
#include "definitions.h"

int shmid[2];
char* shm[2];

void sighandler(int signo) {
	char hold[2];
	int i, status, waitReturn;
	for(i=0;i=2;i++){
		shmdt(shm[i]);
	}
	exit(1);
}

int main(int argc, char** argv){
	signal(SIGALRM, sighandler);
	signal(SIGINT, sighandler);
	int i, referenceCount=0;
	int processNumber = atoi(argv[1]);
	sem_t* memSem, clockSem;
	char semName[25];

	sprintf(semName, "rooneo%d", processNumber);
	if ((memSem = sem_open (semName, O_CREAT, 0644, 1)) == SEM_FAILED) {
			fprintf(stderr,"Failed to open Sem %i.\n", processNumber);
	}
/*
	key_t clockKey = ftok("./keyFile", 1);

	if ((mutexClock = sem_open ("clockRooneo", O_CREAT, 0644, 1)) == SEM_FAILED) {
				fprintf(stderr,"Failed to open clock mutex.\n");
	}
	
	LogicalClock* clock;
	shmid[0] = shmget(clockKey, sizeof(LogicalClock), IPC_CREAT|0700);
	if(shmid[0] < 0){
		fprintf(stderr,"Failed to open clock %i.\n", processNumber);
		exit(-1);
	}
	shm[1] = shmat(shmid[0], NULL, 0);
	if(shm[0] == (char*) -1){
		fprintf(stderr,"Failed to attach clock %i.\n", processNumber);
		exit(-1);
	}
*/
	key_t shmKey = ftok("./keyFile", processNumber+1);
	PageTable* pageTable;
	shmid[1] = shmget(shmKey,sizeof(PageTable) , IPC_CREAT|0700);
	if(shmid[1] < 0){
		fprintf(stderr,"Failed to open shm %i.\n", processNumber);
		exit(-1);
	}
	shm[1] = shmat(shmid[1], NULL, 0);
	if(shm[1] == (char*) -1){
		fprintf(stderr,"Failed to attach process table %i.\n", processNumber);
		exit(-1);
	}
	pageTable = (PageTable*) shm[1];
	for(i=0;i<PROCESS_MAX_MEM;i++){
		pageTable->table[i].pageNumber=rand()%MAX_MEM;
		pageTable->table[i].wasReferenced = false;
		pageTable->table[i].dirty=false;
		pageTable->table[i].referenceCount=0;
	}
	int pageToReference;
	while(true){
		srand(time(NULL));
		pageToReference=pageTable->table[(rand()*processNumber)%PROCESS_MAX_MEM].pageNumber;
		pageTable->request=pageToReference;
		fprintf(stderr,"Process %i requesting page %i\n", processNumber,pageToReference);
		sem_wait(memSem);
		pageTable->table[pageToReference].wasReferenced = true;
		pageTable->table[pageToReference].referenceCount++;
		if(rand()%2) pageTable->table[pageToReference].dirty = true;
		else pageTable->table[pageToReference].dirty = false;
		referenceCount++;
		if(referenceCount > 1001+(rand()%100)) break;
		sleep(rand()%3+1);
	}
	shmdt(shm[0]);shmdt(shm[1]);
	sem_close(memSem);sem_close(mutexClock);
	return 0;
}
