
/* Local Includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAXFILENAMELENGTH 80
#define DEFAULT_INPUTFILENAME "rwinput"
#define DEFAULT_OUTPUTFILENAMEBASE "rwoutput"
#define DEFAULT_BLOCKSIZE 1024
#define DEFAULT_TRANSFERSIZE 1024*100
#define DEFAULT_ITERATIONS 1000000
#define RADIUS (RAND_MAX / 2)

inline double dist(double x0, double y0, double x1, double y1){
    return sqrt(pow((x1-x0),2) + pow((y1-y0),2));
}

inline double zeroDist(double x, double y){
    return dist(0, 0, x, y);
}

int main(int argc, char* argv[]){

    long i;
    long iterations;
    struct sched_param param;
    int policy;
    double x, y;
    double inCircle = 0.0;
    double inSquare = 0.0;
    double pCircle = 0.0;
    double piCalc = 0.0;
	int inputFD;
	int outputFD;
	char inputFilename[MAXFILENAMELENGTH];
	char outputFilename[MAXFILENAMELENGTH];
	char outputFilenameBase[MAXFILENAMELENGTH];
	ssize_t transfersize = 0;
	ssize_t blocksize = 0;
	char* transferBuffer = NULL;
	ssize_t buffersize;
    ssize_t bytesRead = 0;
    ssize_t totalBytesRead = 0;
    int totalReads = 0;
    ssize_t totalBytesWritten = 0;
	iterations = DEFAULT_ITERATIONS;
	transfersize = DEFAULT_TRANSFERSIZE;
	blocksize = DEFAULT_BLOCKSIZE;
	strncpy(inputFilename, DEFAULT_INPUTFILENAME, MAXFILENAMELENGTH);
	strncpy(outputFilenameBase, DEFAULT_OUTPUTFILENAMEBASE, MAXFILENAMELENGTH);
	buffersize = blocksize;
    /* Set policy if supplied */
	if(!strcmp(argv[1], "SCHED_OTHER")){
	    policy = SCHED_OTHER;
	}
	else if(!strcmp(argv[1], "SCHED_FIFO")){
	    policy = SCHED_FIFO;
	}
	else if(!strcmp(argv[1], "SCHED_RR")){
	    policy = SCHED_RR;
	}
	else{
	    fprintf(stderr, "Unhandeled scheduling policy\n");
	    exit(EXIT_FAILURE);
	}
    
    /* Set process to max prioty for given scheduler */
    param.sched_priority = sched_get_priority_max(policy);
    
    /* Set new scheduler policy */
    fprintf(stdout, "Current Scheduling Policy: %d\n", sched_getscheduler(0));
    fprintf(stdout, "Setting Scheduling Policy to: %d\n", policy);
    if(sched_setscheduler(0, policy, &param)){
	perror("Error setting scheduler policy");
	exit(EXIT_FAILURE);
    }
    fprintf(stdout, "New Scheduling Policy: %d\n", sched_getscheduler(0));

    /* Calculate pi using statistical methode across all iterations*/
    for(i=0; i<iterations; i++){
		x = (random() % (RADIUS * 2)) - RADIUS;
		y = (random() % (RADIUS * 2)) - RADIUS;
		if(zeroDist(x,y) < RADIUS){
	    	inCircle++;
		}
		inSquare++;
    }

    /* Finish calculation */
    pCircle = inCircle/inSquare;
    piCalc = pCircle * 4.0;
	piCalc = piCalc;
	piCalc = argc;

	if((inputFD = open(inputFilename, O_RDONLY | O_SYNC)) < 0){
	    perror("Failed to open input file");
	    exit(EXIT_FAILURE);
    }
	
	if((outputFD = open(outputFilename, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) < 0){
       perror("Failed to open output file");
       exit(EXIT_FAILURE);
    }

    /* Print Status */
    fprintf(stdout, "Reading from %s and writing to %s\n",
	inputFilename, outputFilename);
	
	/* Read from input file and write to output file*/
	do{
	    /* Read transfersize bytes from input file*/
	    bytesRead = read(inputFD, transferBuffer, buffersize);
	    if(bytesRead < 0){
	        perror("Error reading input file");
	        exit(EXIT_FAILURE);
	    }
	    else{
	        totalBytesRead += bytesRead;
	        totalReads++;
	    }
		
	}while(totalBytesWritten < transfersize);
	/* Free Buffer */
    free(transferBuffer);

    /* Close Output File Descriptor */
    if(close(outputFD)){
   	    perror("Failed to close output file");
	    exit(EXIT_FAILURE);
	}

    /* Close Input File Descriptor */
    if(close(inputFD)){
 	   perror("Failed to close input file");
   	   exit(EXIT_FAILURE);
	}

    return 0;
}
