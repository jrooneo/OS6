/************************************************
*	$Author: o-rooneo $
*	$Date: 2015/04/30 09:00:29 $
*	$Log: oss.c,v $
*	Revision 1.5  2015/04/30 09:00:29  o-rooneo
*	Final commit. Project as complete as will be. Info in readme
*
*	Revision 1.4  2015/04/30 04:58:18  o-rooneo
*	oss now forks and loops through creating processes
*
*	Revision 1.3  2015/04/27 21:07:21  o-rooneo
*	setup shared memory and semaphores
*
*	Revision 1.2  2015/04/23 00:26:29  o-rooneo
*	added RCS header keywords
*
************************************************/
#include "definitions.h"

int child[MAX_PROCESS_COUNT+1];
int shmid[MAX_PROCESS_COUNT+1];
char* shm[MAX_PROCESS_COUNT+1];
sem_t *sems[MAX_PROCESS_COUNT+1];
int processCount;
int maxProcessCount;

int freeSlot();
void sighandler(int);
int milliToNano(int);
int milliToNano(int);
unsigned int* timeBetween(LogicalClock*, unsigned int, unsigned int, unsigned int*);


int main(int argc, char** argv)
{
	signal(SIGALRM, sighandler);
	signal(SIGINT, sighandler);
	alarm(60);
	srand(time(NULL));

	int i=0, j=0, k=0, l=0;
	int status, semValue;
	unsigned int timeBetweenReturn[2];
	int waitReturn;
	int nextNS;
	int timeSinceLast=0;
	int timeToNextProcess;
	int maxProcessCount = DEFAULT_PROCESS_COUNT+1;
	processCount = 0;
	char procNum[2];
	bool foundPageFault;
	child[0]=-1;
	for(i=1;i<=MAX_PROCESS_COUNT;i++){
		child[i]=0;
	}

	if(argc == 2){
		maxProcessCount = atoi(argv[1])+1;
		if(maxProcessCount > MAX_PROCESS_COUNT+1){
			fprintf(stderr,"This program restricts process count to %i. Process count set to maximum allowable.\n", MAX_PROCESS_COUNT);
			maxProcessCount = DEFAULT_PROCESS_COUNT+1;
		}
		if(maxProcessCount < 1){
			fprintf(stderr,"This program must have at least 1 process. Process count set to default of %i.\n", DEFAULT_PROCESS_COUNT);
			maxProcessCount = DEFAULT_PROCESS_COUNT+1;
		}
	}
	if(argc > 2){
		fprintf(stderr,"This program is desinged to take a single argument for maximum process count.\nCount set to default of %i\n", DEFAULT_PROCESS_COUNT);
	}
	
	bool allocatedFrames[MAX_MEM];
	Page physicalMemory[MAX_MEM];
	for(i=0; i<MAX_MEM; i++){
		allocatedFrames[i]=0;
	}

	PageTable* processTables[maxProcessCount];
	key_t key[maxProcessCount+1];
	
	for(i=0;i<=maxProcessCount;i++){
		key[i] = ftok("./keyFile", i+1);	
	}
	
	//initialize clock
	LogicalClock* clock;
	if((shmid[0] = shmget(key[0], sizeof(LogicalClock), IPC_CREAT|0700)) < 0){
		fprintf(stderr,"Failed to get clock shm in OSS.\n");
	}
	if((shm[0] = shmat(shmid[0], NULL, 0)) == (char*) -1){
		fprintf(stderr,"Failed to attach clock in OSS.\n");
	}
	clock = (LogicalClock*) shmat(shmid[0], NULL, 0);

	clock->seconds = 0;
	clock->nano = 0;

	for(i=1;i<=maxProcessCount;i++){
		shmid[i] = shmget(key[i], sizeof(PageTable), IPC_CREAT|0700);
		if(shmid[i] < 0){
			fprintf(stderr,"Failed to get shm %i.\n", i);
			exit(-1);
		}
		shm[i] = (char *) shmat(shmid[i], NULL, 0);
		if(shm[i] == (char*) -1){
			fprintf(stderr,"Failed to attach process table %i.\n", i);
			exit(-1);
		}
		processTables[i] = (PageTable*) shm[i];
		processTables[i]->pageCount = 0;
	}

	if ((mutexClock = sem_open ("clockRooneo", O_CREAT, 0644, 1)) == SEM_FAILED) {
				fprintf(stderr,"Failed to open clock mutex.\n");
	}

	for(j = 1; j < maxProcessCount; j++) {
		char semName[25];
		sprintf(semName, "rooneo%d", j);
		if ((sems[j] = sem_open (semName, O_CREAT, 0644, 1)) == SEM_FAILED) {
				fprintf(stderr,"Failed to open Sem %i.\n", j);
		}
	}
	int openFrame;
	int firstPosition;
	int openSlot;
	int lastSecond=0;
	timeToNextProcess = (rand()%milliToNano(500))+1;
	nextNS = (rand() % 1000000) + 1000000;
	
	while(true){
		sem_wait(mutexClock);
			if(clock->nano + nextNS>= 1000000000) {
				clock->seconds += 1;
				clock->nano = 0;
				clock->nano = (clock->nano + nextNS) - 1000000000;
			} else {
				clock->nano+=nextNS;
			}
		sem_post(mutexClock);
		timeSinceLast+=nextNS;
		nextNS = (rand() % 1000000) + 1000000;
		openSlot = freeSlot(maxProcessCount);
		//fprintf(stderr,"open slot is %i\n", openSlot);
		if(openSlot > 0 && timeSinceLast > timeToNextProcess){
			//fprintf(stderr,"**********Entered into process section\n");
			
			processTables[openSlot]->processNum=openSlot;
			processTables[openSlot]->creationSeconds=0;
			processTables[openSlot]->creationNano=0;

			timeSinceLast=0;
			//fprintf(stderr,"**********Child creating openSlot = %i.\n", openSlot);
			child[openSlot]=fork();
			if(child[openSlot]==0){
				sprintf(procNum, "%d", openSlot);
				execl("./userProcess","userProcess", procNum, (const char *) 0);
			}
			if(child[openSlot] == -1){
				fprintf(stderr, "%s: Failed to fork on %i", argv[0], openSlot);
				return -1;
			}
			processCount++;
			//fprintf(stderr,"**********Procces %i created.\n", openSlot);
			//fprintf(stderr,"**********Process Count = %i\n", processCount);
			timeToNextProcess = (rand()%milliToNano(500))+1;
		}
		//fprintf(stderr,"**********pre signal loop\n");
		for(j=1;j<maxProcessCount;j++){
			sem_getvalue(sems[j], &semValue);
			if(semValue == 0){
				foundPageFault=true;
				for(k=0; k<MAX_MEM; k++){
					if(physicalMemory[k].pageNumber==processTables[j]->request){
						foundPageFault=false;
						fprintf(stderr,"Successful read for process %i\n", j);
						physicalMemory[k].wasReferenced=true;
						for(l=0;l<PROCESS_MAX_MEM;l++){
							if(processTables[j]->table[l].pageNumber==processTables[j]->request){
								processTables[j]->table[l].wasReferenced==true;
								physicalMemory[k].dirty=processTables[j]->table[l].dirty;
							}
						}
						sem_post(sems[j]);
						break;
					}
				}
				if(foundPageFault){
					fprintf(stderr,"Page fault from process %i\n", j);
					//Find a free frame (from the free frame list)
					for(k=0; k<MAX_MEM+1; k++){
						if(k==MAX_MEM){
							openFrame=-1;
							break;
						}
						if(allocatedFrames[k]==0){
							openFrame=k;
							break;
						}
					}
					if(openFrame==-1){//no frames free, clear stuff
						for(k=0; k<MAX_MEM+1; k++){
							if(!(physicalMemory[k].wasReferenced)){
								openFrame=k;
							}
						}
					}
					//Schedule the disk to read the required page into the newly allocated frame
					
					//Modify the internal table to indicate that the page is in memory
					physicalMemory[k].pageNumber=processTables[j]->request;
					physicalMemory[k].wasReferenced=true;
					physicalMemory[k].dirty=false;
					physicalMemory[k].referenceCount=1;
					//Restart the instruction interrupted by page fault
					sem_post(sems[j]);
				}
			}
			for(k=0; k<MAX_MEM+1; k++){
				physicalMemory[k].wasReferenced=false;
			}
		}
		while((waitReturn = waitpid(-1,&status,WNOHANG)) > 0){
			for(j=1;j<maxProcessCount;j++){
				if(child[j] == waitReturn){
					processCount--;
					//fprintf(stderr,"**********Cleaning up after process %i\n", j);
					child[j]=0;
					processTables[j]->pageCount=0;
					sem_post(sems[j]);
					break;
				}
			}
			//fprintf(stderr,"**********in small wait loop %i", waitReturn);
		}
		//fprintf(stderr,"**********OSS still running\n");
		/*
		if(lastSecond+5 < clock->seconds){
			lastSecond=clock->seconds;
			for(k=0; k<MAX_MEM+1; k++){
				if(allocatedFrames[k]=0)printf(".");
				else printf("x");
				if((k==(int)MAX_MEM/2))printf("\n");
			}
			printf("\n");
		}*/
	}	
	//fprintf(stderr,"**********OSS out of while\n");

	do{
		waitpid(-1,&status,WNOHANG)>0;
	}while(status>0);

	for(i=0;i<=maxProcessCount+1;i++){
		shmdt(shm[i]);
		shmctl(shmid[i], IPC_RMID, NULL);
	}
	return 0;
}

    //pageFault

int freeSlot(int maxProcessCount){
	int i=1;
	for(i=1; i<maxProcessCount; i++){
		if(child[i] == 0){
			return i;
		}
	}
	return 0;
}
unsigned int* timeBetween(LogicalClock* clock, unsigned int seconds, unsigned int nano, unsigned int returnVals[2]){
	if((returnVals[0]=clock->seconds-seconds) < 0) returnVals[0] = 0;
	if((returnVals[1]=clock->seconds-nano) < 0) returnVals[1] = 0;
	return returnVals;
}

int milliToNano(int milliseconds)
{
	return milliseconds*1000000;
}
int nanoToMilli(int nano)
{
	return (int)((nano/1000000)+1);
}

void sighandler(int signo) {
	char hold[2];
	int i, status, waitReturn;
	do{waitpid(-1,&status,WNOHANG)>0;}while(status>0);
	for(i=0;i<=maxProcessCount+1;i++){
		shmdt(shm[i]);
		shmctl(shmid[i], IPC_RMID, NULL);
	}
	sprintf(hold, "%d", processCount);
	execl("./semClean","semClean", hold, (char*)0);
	exit(1);
}
