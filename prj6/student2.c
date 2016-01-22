/*
 * student.c
 * Multithreaded OS Simulation for CS 2200, Project 4
 *
 * This file contains the CPU scheduler for the simulation.  
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "os-sim.h"
#include "student.h"

/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t **current;
static pthread_mutex_t current_mutex;
static pthread_mutex_t ready_mutex;
static pthread_cond_t is_empty;
static pcb_t *head;
static int scheduler_type;
static int time_slice;
static int cpu_count;
static int in_idle;
int size() {
	int i = 0;
	pcb_t *temp;
	if(head == NULL) return 0;
	for(temp = head; temp->next != NULL; temp = temp->next) {
		printf("%s / ", temp->name);
		i++;
	}
	printf("\n");
	
	return i;
}

void add_list(pcb_t* newQ) {
	pcb_t* temp;
	newQ->next = NULL;
	if(head == NULL) {
		head = newQ;
	} else {
		for(temp = head; temp->next != NULL; temp = temp->next);
		temp->next = newQ;
	}
}

pcb_t* remove_list() {
	pcb_t* temp;
	
	if(head == NULL) {
		return NULL;
	} else {
		temp = head;
		head = head->next;
	}
	return temp;
}	

pcb_t* remove_from_back() {
	pcb_t* temp, *val;
	if(head == NULL) {
		return NULL;
	} else if (head->next == NULL) {
		head = NULL;	
		return NULL;
	} else {

		for(temp = head; temp->next->next != NULL; temp = temp->next);
		val = temp->next;
		temp->next = NULL;
	}
	return val;
}

pcb_t* get_highest_priority() {
	pcb_t *temp, *max;

	if(head == NULL) {
		return NULL;
	} else {
		max = head;
		for(temp = head; temp->next != NULL; temp = temp->next) {
			if(max->static_priority < temp->static_priority) {
				max = temp;
			}
		}
	}

	return max;
}	




/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which 
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *	The current array (see above) is how you access the currently running process indexed by the cpu id. 
 *	See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information 
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{

	pthread_mutex_lock(&ready_mutex);
	pcb_t* newQ = remove_list();
	pthread_mutex_unlock(&ready_mutex);
	if(newQ == NULL) {
		context_switch(cpu_id, NULL, time_slice); 
	
	} else {
		pthread_mutex_lock(&ready_mutex);
		newQ->state = PROCESS_RUNNING;
		pthread_mutex_unlock(&ready_mutex);
		pthread_mutex_lock(&current_mutex);
		current[cpu_id] = newQ;
		pthread_mutex_unlock(&current_mutex);
		context_switch(cpu_id, newQ, time_slice); 

	} 
}


/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
	
	pthread_mutex_lock(&ready_mutex);
	in_idle++;
	while(head == NULL){
		pthread_cond_wait(&is_empty, &ready_mutex);
	}
	in_idle--;
	pthread_mutex_unlock(&ready_mutex);
	schedule(cpu_id);
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{
	pcb_t *running_pcb;
	running_pcb = current[cpu_id];
	add_list(running_pcb);
	schedule(cpu_id);


}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
	pthread_mutex_lock(&current_mutex);
	current[cpu_id]->state = PROCESS_WAITING;
	pthread_mutex_unlock(&current_mutex);
	schedule(cpu_id);	

}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
	pthread_mutex_lock(&current_mutex);
	current[cpu_id]->state = PROCESS_TERMINATED;
	pthread_mutex_unlock(&current_mutex);
	schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is static priority, wake_up() may need
 *      to preempt the CPU with the lowest priority process to allow it to
 *      execute the process which just woke up.  However, if any CPU is
 *      currently running idle, or all of the CPUs are running processes
 *      with a higher priority than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for 
 * 	its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
	int victim = -1;
	int i;
	pthread_mutex_lock(&ready_mutex);
	pthread_mutex_lock(&current_mutex);
	if(scheduler_type == 3) {
		for(i = 0; i < cpu_count; i++) {
			if(current[i] !=NULL) {
				if((current[i]->static_priority) < process->static_priority) {
					victim = i;
				}
			}
		}
		if(victim != -1) {
			pthread_mutex_unlock(&ready_mutex);
			pthread_mutex_unlock(&current_mutex);
			if(head != NULL) {
				force_preempt(victim);
			}
			pthread_mutex_lock(&current_mutex);
			pthread_mutex_lock(&ready_mutex);

		}
	}
	process->state = PROCESS_READY;
	add_list(process);
	pthread_cond_signal(&is_empty);
	pthread_mutex_unlock(&current_mutex);
	pthread_mutex_unlock(&ready_mutex);


}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -p command-line parameters.
 */
int main(int argc, char *argv[])
{

	in_idle = 0;
	cpu_count = 0;
    /* Parse command-line arguments */
    if (argc < 2 || argc > 4)
    {
        fprintf(stderr, "CS 2200 Project 4 -- Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p ]\n"
            "    Default : FIFO Scheduler\n"
            "         -r : Round-Robin Scheduler\n"
            "         -p : Static Priority Scheduler\n\n");
        return -1;
    }
    	cpu_count = atoi(argv[1]);


	scheduler_type = 0;
	if(argc == 2){
		scheduler_type = 1;
		time_slice = -1;
	} else {
		if(argv[2][1] == 'r'){
			scheduler_type = 2;
printf("oh %s\n", argv[3]);
			time_slice = atoi(argv[3]);//h8 * pow(10,8);

		}
		if(argv[2][1] == 'p')
			scheduler_type = 3;
	}
	if(scheduler_type == 0) {
		printf("wrong input!");
		return -1;
	}

	printf("cpu : %d sc : %d\n", cpu_count, scheduler_type);
	 

    /* FIX ME - Add support for -r and -p parameters*/


	head = NULL;
    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);


	pthread_mutex_init(&ready_mutex, NULL);
	pthread_cond_init(&is_empty,NULL);
    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}


