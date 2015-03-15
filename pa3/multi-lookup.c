//The main file

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

//Need to init some muetexes in here for files
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t flock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rlock = PTHREAD_MUTEX_INITIALIZER;

//Declare a gloabal queue for usage
queue q;

//Should have returns for the different errors that are given in the assignment.
void* load(void* infile){ //Function to handle building a queue from the file.
		
}

//These functions should have different returns for the errors that were outlined in the assignment file. 
void* reswrite(void* input){ //Function to resolve hostnames and then write to the file.)

}
void main(int argc, char *argv){
		//create the output file and check to make sure it worked
		FILE *ouput;
		output = fopen("output.txt", "w");
		if(!output){
				printf("Error creating output file");
				exit(1);
		}
		fclose(f);

		//Check to see if the number of input files exceeds the max allowed
		if(argc > MINPUT+1){
			   printf("Error: Too many input arguments");
			   exit(1);
		}	   

		//Iterate over the *argv array giving the names of files and performing operations on each one. 

}
