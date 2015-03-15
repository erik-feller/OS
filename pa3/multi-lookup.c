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
#include <sys/sysinfo.h>

//Defined limits
#define QMAXSIZE 10
#define MINPUT 10
#define MRTHREADS 10
#define MINRTHREADS 2
#define MAXNLEN 1025

//Find the number of cores on this machine
int num;
num = (int)get_nprocs();

//Need to init some muetexes in here for les
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t flock = PTHREAD_MUTEX_INITIALIZER;

//Declare gloabal vars
queue q; //queue to hold the data read from the les
char requestFinished; //variable to track if req are done or not

//Define a struct to allow storage of stuff on the heap. Stack instances won't work
typedef struct {
		char *data;
}heapStore;

void* req(void* inle){ //Function to handle building a queue from the input les.
		//Use the heap storage variable to retrive the input le that will be used for this thread
		char *lename;
		char lineread[1025];
		heapStore *secondary = inle; //This line to kill a warning
		lename = secondary->data;
		FILE *le = fopen(lename, "r");
		
		//Do a check to make sure the le exitst
		if (le == NULL){
				printf("Input le is not valid\n");
				exit(1);
		}		

		//Now loop over reading the le until it contains no unread data
		while(fscanf(le, "%1025s", lineread) > 0){
				pthread_mutex_lock(&qlock);
				while(queue_is_full(&q)){
						pthread_mutex_unlock(&qlock);
						usleep(16);
						pthread_mutex_unlock(&qlock);
				}
				//Push the lines read onto the queue
				int queueval = queue_push(&q, strdup(lineread));
				if(queueval){
						printf("Queue returned with errors on string push");
						exit(1);
				}
		}	
		return NULL;

}

void* resolver(void* input){ //Function to resolve hostnames and then write to the le.
		(void) input; //This is to kill a warning
		//set a loop to wait until all the input has been read or if the queue is empty
		while(!requestFinished | !queue_is_empty(&q)){
				char *handle;
				char ip[INET6_ADDRSTRLEN];
				pthread_mutex_lock(&qlock);
				if(!queue_is_empty(&q)){
						handle = queue_pop(&q);
						pthread_mutex_unlock(&qlock);
				}
				else{
						//If the queue is empty just wait some time. Skips rest of loop
						pthread_mutex_unlock(&qlock);
						usleep(10);
						continue;
				}
				
				//Now remove the newline from the hostname
				char *curr;
				if((curr = strchr(handle, '\n')) != NULL){
						*curr = '\0';
				}
				if(dnslookup(handle, ip, 64)){
						*ip = '\0';
				}
				
				//Write to output
				pthread_mutex_lock(&flock);
				FILE *le = fopen("results.txt", "a");
				fprintf(le, "%s, %s\n", handle, ip);
				fclose(le); //Close that le
				pthread_mutex_unlock(&flock);
				free(handle);
		}
		return NULL;

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
				error = pthread_create(&(requesterThreads[i]), NULL, req, &heapStuff[i]);
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
