#include <semaphore.h>
#include <stdio.h>
int main(int argc, char** argv){
	int semCount = 12;
	if(argc == 2){
		semCount = atoi(argv[1]);
		if(semCount < 1){
			semCount=18;
		}
	}
	int result, i;
	if ((result = sem_unlink ("clockRooneo")) == -1) {
				//printf("Failed to close clock mutex.\n");
	}

	for(i = 1; i < semCount; i++) {
		char semName[25];
		sprintf(semName, "rooneo%d", i);
		if ((result = sem_unlink (semName)) == -1) {
				//printf("Failed to close Sem %i.\n", i);
		}
	}
}
