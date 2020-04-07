/*
AUbatch- A Pthread-based Operating Systems

The following program is a creation of my own. However, there were many new things like strtok, dealing with structs handling synchronizations and condition variables for which I relied on Dr. Qin's implementation. Following that I had some issues dealing with the more sophisticated table driven approach discussed by Dr. Qin So I switched to an easy approach of gradually building over things incrementally
*/

#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#define MAX_JOB 100				
#define NUM_JOB 50

pthread_t scheduler;				// For scheduler thread
pthread_t dispatcher;				// For dispatcher thread
pthread_mutex_t cmd_queue_lock;  	// mutex lock
pthread_cond_t cmd_buf_not_full; 	// condition variable to indicate buffer not full
pthread_cond_t cmd_buf_not_empty; 	// condition variable to indicate buffer not empty

int job_count = 0;			// number of inserted jobs in the queue
int total_job_system=0;		// to find the average waiting time and turn around time we require the total number of jobs
							// the system has seen
enum policies{fcfs, sjf, priority}; // to hold the policy user has switched to
enum policies policy = fcfs;
double waitTime=0.0;				// global variable to hold the waiting time of the system
double expWaitTime=0.0;
double turnAroundTime=0.0;		// global variable to hold the turn around time of the system

struct JobQueue			//The structure is similar to the one discussed in class 
{  
	int id;
	char job_name[20];			//consider as an Id for the process -> user given
	int cpuTime;			//the required CPU time for execution -> user given
	int priority;			//priority of the job -> user given
	time_t arrivalTime;		//arrival time -> computed
	time_t executionTime;		//execution time -> computed
	time_t completionTime;		//completion time -> computed
	char job_status[20];		//job is running, waiting or executed
};

struct JobQueue job[MAX_JOB];	//array of jobs of type JobQueue
int head =0;		//head to indicate the point of entry for the next job i.e the place where the scheduler thread shoul keep 
					//a new job in the queue
int tail = 0;		// points to a location in the job queue which tells the dispatcher which is the next job to execute
		
//displays the help menu			
void cmd_helpMenu(){
	printf("run <jobname> <time> <pri>: submit a job named <jobname>,\n\t\t\t    execution time is <time>,\n\t\t\t    priority is <pri>.");
	printf("\nlist: display the job status.");
	printf("\nfcfs: change the scheduling policy to FCFS.");
	printf("\nsjf: change the scheduling policy to SJF.");
	printf("\npriority: change the scheduling policy to priority.");
	printf("\ntest <benchmark> <policy> <num_of_jobs> <priority_levels> <min_CPU_time> <max_CPU_time>");
	printf("\nquit: exit AUbatch");
}

// Function to calculate performance 
// used to populate waitTime & turnAroundTime shared variables
// Using diff_t : function to find the difference between two times
// diff_t = difftime(end_t, start_t);
void performance_evaluation(time_t arrivalTime, time_t executionTime, time_t completionTime){
	
	double dift_WT, dift_TAT;
	dift_WT = difftime(executionTime, arrivalTime);
	dift_TAT = difftime(completionTime, executionTime);
	/*
	printf("\n%s",ctime(&arrivalTime));
	printf("\n%s",ctime(&executionTime));
	printf("\n%s",ctime(&completionTime));
	
	printf("\n%f",dift_WT);
	printf("\n%f",dift_TAT);
	*/
	
	waitTime = waitTime + dift_WT;
	turnAroundTime = turnAroundTime + waitTime + dift_TAT;
}

// The function performs a sort on the queue based upon the job that came first 
// This is achieved by comparing the id's of each job in the queue
// The swapping of structure is a tedious task and can be made simpler by using operator overloading. 
// However C does not supports operator overloading and hence we need to swap each attribute of a structure
// Source: Stack overflow discussuion on swapping structure elemnts
// The algorithm form performing the sort is not a dynamic one. The commonly used Insertion sort is modified here to sort on the basis of jobId
int cmd_fcfs(){
	int jobID;				// to hold the temp job structure info
	char jobName[20];		
	int jobCPU;
	int jobPRIO;
	time_t jobTime;			
	int i,j;				

	for (i=tail+1; i<head; i++){
		jobID = job[i].id;
		strcpy(jobName , job[i].job_name);
		jobTime = job[i].arrivalTime;
		jobCPU = job[i].cpuTime;
		jobPRIO = job[i].priority;
		
		j = i-1;
		while((j>=1) && (job[j].id>jobID)){
			job[j+1].id = job[j].id;
			strcpy(job[j+1].job_name ,job[j].job_name);
			job[j+1].arrivalTime = job[j].arrivalTime;
			job[j+1].cpuTime = job[j].cpuTime;
			job[j+1].priority = job[j].priority;				
			j = j-1;
		}
		
		job[j+1].id = jobID;
		strcpy(job[j+1].job_name , jobName);
		job[j+1].arrivalTime = jobTime;
		job[j+1].cpuTime = jobCPU;
		job[j+1].priority = jobPRIO;
	}
}

// The function performs a sort on the queue based upon the job that came first 
// This is achieved by comparing the id's of each job in the queue
// The swapping of structure is a tedious task and can be made simpler by using operator overloading. 
// However C does not supports operator overloading and hence we need to swap each attribute of a structure
// Source: Stack overflow discussuion on swapping structure elemnts
// The algorithm form performing the sort is not a dynamic one. The commonly used Insertion sort is modified here to sort on the basis of the job that needs the shortest time for execution i.e. CPU time 
int cmd_sjf(){
	int jobID;
	char jobName[20];
	int jobCPU;
	int jobPRIO;
	time_t jobTime;
	int i,j;
	
	for (i=tail+1; i<head; i++){
		jobID = job[i].id;
		strcpy(jobName , job[i].job_name);
		jobTime = job[i].arrivalTime;
		jobCPU = job[i].cpuTime;
		jobPRIO = job[i].priority;
		
		j = i-1;
		while((j>=1) && (job[j].cpuTime>jobCPU)){
			job[j+1].id = job[j].id;
			strcpy(job[j+1].job_name ,job[j].job_name);
			job[j+1].arrivalTime = job[j].arrivalTime;
			job[j+1].cpuTime = job[j].cpuTime;
			job[j+1].priority = job[j].priority;				
			j = j-1;
		}
		
		job[j+1].id = jobID;
		strcpy(job[j+1].job_name , jobName);
		job[j+1].arrivalTime = jobTime;
		job[j+1].cpuTime = jobCPU;
		job[j+1].priority = jobPRIO;
	}
}

// The function performs a sort on the queue based upon the job that came first 
// This is achieved by comparing the id's of each job in the queue
// The swapping of structure is a tedious task and can be made simpler by using operator overloading. 
// However C does not supports operator overloading and hence we need to swap each attribute of a structure
// Source: Stack overflow discussuion on swapping structure elemnts
// The algorithm form performing the sort is not a dynamic one. The commonly used Insertion sort is modified here to sort on the basis of priority. Note a priority of a job with a larger num is greater than that of a job with a smaller num 
int cmd_priority(){
	int jobID;
	char jobName[20];
	int jobCPU;
	int jobPRIO;
	time_t jobTime;
	int i,j;
	
	for (i=tail+1; i<head; i++){
		jobID = job[i].id;
		strcpy(jobName , job[i].job_name);
		jobTime = job[i].arrivalTime;
		jobCPU = job[i].cpuTime;
		jobPRIO = job[i].priority;
		
		j = i-1;
		while((j>=1) && (job[j].priority<jobPRIO)){
			job[j+1].id = job[j].id;
			strcpy(job[j+1].job_name ,job[j].job_name);
			job[j+1].arrivalTime = job[j].arrivalTime;
			job[j+1].cpuTime = job[j].cpuTime;
			job[j+1].priority = job[j].priority;				
			j = j-1;
		}
		
		job[j+1].id = jobID;
		strcpy(job[j+1].job_name , jobName);
		job[j+1].arrivalTime = jobTime;
		job[j+1].cpuTime = jobCPU;
		job[j+1].priority = jobPRIO;
	}
}

int cmd_run(char input_cmd[], enum policies p){
	pthread_mutex_lock(&cmd_queue_lock);		// gain a mutex lock 
	char command[100];
	strcpy(command,input_cmd);
	char *word_token = strtok(command, " ");
	int w=1;
	while (word_token != NULL)
	{
		switch(w){
			case 1:			// A valid run command will have run followed by job -> time -> priority
				break;		// Ignoring run
				
			case 2:			//extracting the name of the executable
				sscanf(word_token, "%s", &job[head].job_name);
				break;
			
			case 3:			//extracting the required CPU time
				sscanf(word_token, "%d", &job[head].cpuTime);
				break;
				
			case 4:			//extracting the priority
				sscanf(word_token, "%d", &job[head].priority);
				break;	
		}
		w++;
		word_token = strtok(NULL, " ");
	}
	if(w!=5){
		printf("Usage: run <job> <time> <pri>");
		return -1;
	}
	
	printf("Job %s was submitted.",job[head].job_name);		// populating info about the job
	time(&job[head].arrivalTime);					// function to note job arrival the time
	strcpy(job[head].job_status,"Waiting");
	job[head].id = head+1;			//assigning Id to a job
	job_count++;					// To denote number of jobs in the queue
	printf("\nTotal number of jobs in the queue: %d",job_count);
	expWaitTime = expWaitTime + waitTime;		//expected waiting time != waiting time, as we have jobs waiting in the queue
	printf("\nExpected waiting time: %f seconds", expWaitTime);
	switch(p){
		
		case 0:
			printf("\nScheduling Policy: FCFS");
			cmd_fcfs();		//call to order via fcfs
			break;	
		
		case 1:
			printf("\nScheduling Policy: SJF");
			cmd_sjf();		//call to order via sjf
			break;	
		
		case 2:
			printf("\nScheduling Policy: PRIO");
			cmd_priority(); //call to order via priority
			break;		
	}
	
	head = (head+1) % MAX_JOB;	//head++;		// Since circular job queue
	if (head == MAX_JOB){
		head = 0;
	}
	total_job_system++;		// keeps a track of number of jobs that are seen by the system
	pthread_cond_signal(&cmd_buf_not_empty);  //signal for someone waiting on buf to be not empty
	pthread_mutex_unlock(&cmd_queue_lock);	//release the mutex lock
}

// To extract the HH:MM:SS from the time that we get which is of the format
// Day Month Date HH:MM:SS Year
// reffered implementation at https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
char* getTimeFromCTime(time_t time){
	char timeStr[25]="";
	//printf("Entering: %s",  ctime(&time));
	//strcpy(timeStr, ctime(&time));
	//printf("Size of time: %d",sizeof(time));
	//printf("Size of time: %d",sizeof(ctime(&time)));
	ctime_r(&time, timeStr);
	//printf("Entering timeStr: %s",  timeStr);
	//printf("Size of timeStr: %d",sizeof(timeStr));
	
	char* token; 
    char* rest = timeStr; 
  	int i=0;
    while ((token = strtok_r(rest, " ", &rest))) {
    	i++;
    	if(i==4){
    		//printf("%s\n", token);
    		return token;
    	}
    }	
	return NULL;
}

// display the queue. we need to get a lock prior to the call as the ques is a shared resource and if we don't get a lock it may happen that dispatcher may execute a job or schedule may create a new job and assign to the queue
void cmd_display(enum policies p){
	int i=0;
	printf("\nTotal number of jobs in the queue: %d",job_count);
	switch(p){	
		case 0:
			printf("\nScheduling Policy: FCFS");
			break;	
		
		case 1:
			printf("\nScheduling Policy: SJF");
			break;	
		
		case 2:
			printf("\nScheduling Policy: PRIO");
			break;		
	}	
	printf("\nName\tCPU_Time\tPri\tArrival_time\tProgress");
	for(i=tail; i<head; i++){
		printf("\n%s\t%d\t\t%d\t\t%s\t%s",job[i].job_name,job[i].cpuTime,job[i].priority,getTimeFromCTime(job[i].arrivalTime),job[i].job_status);			
	}	
}

// Function to generate a priority value within a range [l,u] both included
int getRandom_Priority(int l, int u){
	int priority = (rand() % (u-l+1)) + l;
	return priority;
}

// Function to generate a CPU time within a range [l,u] both included
int getRandom_CPU(int l, int u){
	int CPU_time = (rand() % (u-l+1)) + l;
	return CPU_time;
}

// Function to perform automation testing
int cmd_test(char input_cmd[]){
	char command[100];
	strcpy(command,input_cmd);					// string function to copy input_cmd to command
	char *word_token = strtok(command, " ");
	
	int priority_levels, num_of_jobs, minCPU, maxCPU;
	char jobName[20];
	char input_cmd_run[50];
	char timeStr[10];
	char prioStr[10];
	
	expWaitTime = 0.0;
	
	int w=1;
	int i;
	while (word_token != NULL)			// same as extractTime
	{
		switch(w){
			case 1:			// A valid test command will have test followed by job -> policy -> num_of_jobs -> priority_levels
				break;		// -> min_CPU_time -> max_CPU_time
				
			case 2:			//extracting the jobName
				sscanf(word_token, "%s", &jobName);
				break;
			
			case 3:			//extracting the policy 
				//printf("%s",word_token);
				if(strcmp(word_token,"fcfs")==0){
					policy = fcfs;
				}
				else if(strcmp(word_token,"sjf")==0){
					policy = sjf;
				}
				else if(strcmp(word_token,"priority")==0){
					policy = priority;
				}
				break;
				
			case 4:			//extracting the number of jobs priority levels
				sscanf(word_token, "%d", &num_of_jobs);
				break;	
				
			case 5:			//extracting the priority levels
				sscanf(word_token, "%d", &priority_levels);
				break;
			
			case 6:			//extracting the minCPU time
				sscanf(word_token, "%d", &minCPU);
				break;
			
			case 7:			//extracting the maxCPU time
				sscanf(word_token, "%d", &maxCPU);
				break;	
		}
		w++;
		word_token = strtok(NULL, " ");
	}
	if(w!=8){
		printf("Usage: test <benchmark> <policy> <num_of_jobs> <priority_levels> <min_CPU_time> <max_CPU_time>");
		return -1;
	}
	
	//forming the input command formated and expected by cmd_run()
	for(i=0; i<num_of_jobs; i++){			// calling the cmd_run
		strcpy(input_cmd_run, "run ");
		
		strcat(input_cmd_run, jobName);
		sprintf(timeStr, " %d", getRandom_CPU(minCPU, maxCPU));
		strcat(input_cmd_run, timeStr);
		sprintf(prioStr, " %d", getRandom_Priority(0,priority_levels));
		strcat(input_cmd_run, prioStr);
		
		//printf("> \n%s : %d\n",input_cmd_run,policy);
		sleep(2);
		printf("\n> %s\n",input_cmd_run,policy);
		cmd_run(input_cmd_run, policy);
	}
	printf("\n>list");
	cmd_display(policy);
	memset(command,0,sizeof(command));
}

//The scheduler thread
void *scheduling(){
	//printf("\nInside Scheduler Thread");
	char command[100];
    char *cmd;
    size_t input_size = 64;			
	cmd = (char *)malloc(input_size * sizeof(char));		// as given in the Dr. Qin refernce code 
	
	int i;
	float avgTurnAroundTime;
	float avgWaitingTime;
	float throughput;
	
	for(i=0; i<NUM_JOB; i++){
		pthread_mutex_lock(&cmd_queue_lock);		// latching the resource
		while (job_count == MAX_JOB) {
			/*Waits if the buffer is full and unblocks when there is a space created i.e buffer is not full*/
			pthread_cond_wait(&cmd_buf_not_full, &cmd_queue_lock);
		}
		pthread_mutex_unlock(&cmd_queue_lock);	//releasing the mutex so that another process can claim it
		printf("\n>");		
		getline(&cmd,&input_size,stdin);		// getting input from stdin
		
		strncpy(command,cmd,strlen(cmd)-1);		
		// strcmp function to compare two strings
		if(strcmp(command,"help")==0 || strcmp(command,"h")==0 || strcmp(command,"?")==0){
			cmd_helpMenu();
		}
		
		else if(strstr(command,"run")){				// comparing the string run and the ipt command
			cmd_run(command,policy);
		}
		
		else if(strcmp(command,"list")==0){
			pthread_mutex_lock(&cmd_queue_lock);
			cmd_display(policy);
			pthread_mutex_unlock(&cmd_queue_lock);
		}
		
		else if(strcmp(command,"fcfs")==0){
			pthread_mutex_lock(&cmd_queue_lock); //get a mutex lock
			policy = fcfs;		// changing the enum variable to schedule the new jobs by the specified policy
			cmd_fcfs();			// call to fcfs
			printf("\nScheduling policy is switched to FCFS. All the %d waiting jobs have been rescheduled.",job_count);
			pthread_mutex_unlock(&cmd_queue_lock); //release a mutex lock
		}
		
		else if(strcmp(command,"sjf")==0){
			pthread_mutex_lock(&cmd_queue_lock);
			policy = sjf;	// changing the enum variable to schedule the new jobs by the specified policy
			cmd_sjf();		//call to sjf
			printf("\nScheduling policy is switched to SJF. All the %d waiting jobs have been rescheduled.",job_count);
			pthread_mutex_unlock(&cmd_queue_lock);
		}
		
		else if(strcmp(command,"priority")==0){
			pthread_mutex_lock(&cmd_queue_lock);
			policy = priority;	// changing the enum variable to schedule the new jobs by the specified policy		
			cmd_priority();		//call to priority
			printf("\nScheduling policy is switched to PRIORITY. All the %d waiting jobs have been rescheduled.",job_count);
			pthread_mutex_unlock(&cmd_queue_lock);
		}
		
		else if(strstr(command,"test")){				
			cmd_test(command);		// run function get the lock so no need to get a lock over here and then call test
		}
		
		else if(strcmp(command,"quit")==0 || strcmp(command,"q")==0){  //print the performance infor
			avgTurnAroundTime = turnAroundTime/total_job_system;
			avgWaitingTime = waitTime/total_job_system;
			throughput = 1.0/avgTurnAroundTime;
			printf("\nTotal jobs submitted : %d",total_job_system);
			printf("\nAverage waiting time : %f",avgWaitingTime);
			printf("\nAverage turn around time : %f",avgTurnAroundTime);
			printf("\nThroughput : %f",throughput);
			exit(0);	
		}
		
		else{
			i = i - 1;
			printf("Command not found, Type help to display menu");
		}
		
		
		memset(command,0,sizeof(command));		// to reuse the memory	
	}
	return NULL;
}

void *dispatching(){
	//printf("\nInside Dispatching Thread");
	int i;
	double diff_t;

	for(i=0; i<NUM_JOB; i++){
		//FILE * fp;
		//fp = fopen ("log.txt", "w+");
		sleep(10);			// deliberately made to sleep so that the producer can generate a job and schedule it while the 
							// dispatcher is sleeping. Discussed in brief in lessons learned
		//printf("\nProcessing: %d",i);	
		pthread_mutex_lock(&cmd_queue_lock);	//lock the resource, claim the lock
		while (job_count == 0) {
			/*Waits if the buffer is empty and unblocks when there is a job scheduled i.e buffer is not empty*/
            pthread_cond_wait(&cmd_buf_not_empty, &cmd_queue_lock);
        }
        time(&job[tail].executionTime);
        strcpy(job[tail].job_status,"Run");
        pid_t proc = fork();			// Why fork is needed is discussed in lessons learned section
		if (proc==0){	
			execv("./sample_job",NULL);	// executing the sample_job by the child	
		}
		else if(proc > 0){
			sleep(job[tail].cpuTime);		// sleep for the cpu time
			//sleep(1);
			time(&job[tail].completionTime);
			//fprintf(fp, "\nExecuting %s with %d : %s", job[tail].job_name,job[tail].cpuTime, ctime(&job[tail].arrivalTime));
			//fclose(fp);
			//printf("\nExecuting %s with %d : %s",job[tail].job_name,job[tail].cpuTime, ctime(&job[tail].arrivalTime));				
			strcpy(job[tail].job_status,"Executed");		// mark status as executed
			performance_evaluation(job[tail].arrivalTime,job[tail].executionTime,job[tail].completionTime);
			
			//diff_t = difftime(job[tail].completionTime, job[tail].executionTime);
			//printf("\n%f",diff_t);
			
			job_count--;
		    tail = (tail+1) % MAX_JOB;		// since circular queue
		    if (tail == MAX_JOB){			//just for safety, with modulo in place this won't execute
				tail = 0; 
			}
		    
			pthread_cond_signal(&cmd_buf_not_full); //signal someone waiting for a space in the queue to be created	
			pthread_mutex_unlock(&cmd_queue_lock); 	//release the mutex lock
		}
	}
}	

int main()
{
	int scheduler_thread_ret;
	int dispatcher_thread_ret;
	
	printf("Welcome to Satyams's batch job scheduler Version 1.0\nType 'help' to find more about AUBatch commands.\n");

	// creating two threads producer and consumer
	// i.e scheduler and dispatcher
	scheduler_thread_ret=pthread_create(&scheduler,NULL,scheduling,NULL);
	dispatcher_thread_ret=pthread_create(&dispatcher,NULL,dispatching,NULL);
	
	//check if the threads are created
	if(scheduler_thread_ret != 0 || dispatcher_thread_ret != 0){
		printf("Failed to create two threads");
		return -1;
	}	
	
	// initialize the mutex condition variables
	pthread_mutex_init(&cmd_queue_lock, NULL);
	pthread_cond_init(&cmd_buf_not_full, NULL);
	pthread_cond_init(&cmd_buf_not_empty, NULL);
	
	// wait for the scheduler and dispatcher thread to join
	pthread_join(scheduler,NULL);
	pthread_join(dispatcher,NULL);
	
	printf("\n\n");
	return 0;
}
