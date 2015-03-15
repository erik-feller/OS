//The main le

//Includes
#include "queue.h" //This is given by the OS prof
#include "util.h" //Also given by the OS prof
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>

//Defined limits
#define QMAXSIZE 10
#define MINPUT 10
#define MRTHREADS 10
#define MINRTHREADS 2
#define MAXNLEN 1025

//Find the number of cores on this machine
int NUMCORES = sysconf(_SC_NPROCESSORS_ONLN);

//Need to init some muetexes in here for les
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t flock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rlock = PTHREAD_MUTEX_INITIALIZER;

//Declare gloabal vars
queue q; //queue to hold the data read from the les
char requestFinished; //variable to track if req are done or not

//Define a struct to allow storage of stuff on the heap. Stack instances won't work
typedef struct {
		char *data;
}heapStore;

void* req(void* inle){ //Function to handle building a queue from the input les.
		//Use the heap storage variable to retrive the input file that will be used for this thread
		char lename[MAXNLEN];
		char lineread[1025];
		lename = inle->data;
		FILE *le = fopen(lename, 'r');
		
		//Do a check to make sure the file exitst
		if (le == NULL){
				printf("Input le is not valid\n");
				exit(1);
		}		

		//Now loop over reading the file until it contains no unread data
		while(fscanf(file, "%1025s", line) > 0){
				pthread_mutex_lock(&qlock);
				while(queue_is_full(&q)){
						pthread_mutex_unlock(&qlock);
						usleep(16);
						pthread_mutex_unlock(&qlock);
				}
				//Push the lines read onto the queue
				int queueval = queue_push(&q, strdup(line));
				if(queueval){
						printf("Queue returned with errors on string push");
						exit(1);
				}
		}	
		return NULL;

}

void* resolver(void* input){ //Function to resolve hostnames and then write to the le.

}
void main(int argc, char *argv){
		//create the output le and check to make sure it worked
		FILE *ouput;
		output = fopen("output.txt", "w");
		if(!output){
				printf("Error creating output ile");
				exit(1);
		}
		fclose(f);

		//Check to see if the number of input les exceeds the max allowed
		if(argc > MINPUT+1){
			   printf("Error: Too many input arguments");
			   exit(1);
		}	   

		//Now create threads to request based on the number of les passed in.
		queue_init(&q, 10);
	    int numThreads = argc-1;
		pthread_t *reqThreads = malloc(sizeof(pthread_t)*numThreads);
		requestFinished = 0;		
		int i;
		int error;
		heapStore *heapStuff = malloc(sizeof(heapStore)*numThreads); //This is allocating space to store the input les on the heap.
		for(i = 0; i < numThreads; i++){
				heapStuff[i].data = argv[i+1];
				error = pthread_create(&(requesterThreads[i]), NULL, request, &heapStuff[i]);
				if(error){
					printf("pthread couldn't be created.\nError No. %d", error);
				}
		}

		//Calculate the number of resolver threads to use based on CPU
		int threadsUtilized;
		if(NUMCORES == 0){
				threadsUtilized = 2;
		}
		else{
				threadsUtilized = NUMCORES;
		}		
		
		//Create the resolver threads
		pthread_t resolverThreads[threadsUtilized];
		for(i = 0; i < threadsUtilized; i++){
				error = pthread_create(&(resolverThreads[i]), NULL, resolver, NULL);
				if(error){
						printf("pthread couldn't be created.\nError No. %d", error);
				}
		}

		//Now just execute through the threads

		//Now clean up the stuff
		queue_cleanup(&q);
		free(heapStuff);
		free(requesterThreads);
		free(resolverThreads);
					
}
