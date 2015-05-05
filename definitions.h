/************************************************
*	$Author$
*	$Date$
*	$Log$
************************************************/
#ifndef DEFS
#define DEFS



#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

typedef int bool;
enum { false, true };

#define PAGE_COUNT 256
#define MAX_PROCESS_COUNT 18
#define DEFAULT_PROCESS_COUNT 12
#define MAX_MEM 256
#define PROCESS_MAX_MEM 32
#define PAGE_SIZE 1

typedef struct Page{
	int pageNumber;
	bool wasReferenced;
	bool dirty;
	int referenceCount;
}Page;

typedef struct PageTable{
	int processNum;
	int request;
	unsigned int creationSeconds;
 	unsigned int creationNano;
	int pageCount;
	Page table[PROCESS_MAX_MEM];
}PageTable;

typedef struct LogicalClock{
  unsigned int seconds;
  unsigned int nano;
}LogicalClock;

sem_t *mutexClock;
#endif
