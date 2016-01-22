/*
 * student.c
 * Multithreaded OS Simulation for CS 2200, Project 6
 * Summer 2015
 *
 * This file contains the CPU scheduler for the simulation.
 * Name:
 * GTID:
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

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
static pthread_cond_t isEmpty;
static pcb_t* head;
static int time_slice = -1;
static int priority = 0;
static int cpu_count;
void put(pcb_t* newP) {
	pcb_t* curr;
	newP->next = NULL;
	if (head == NULL) {
		head = newP;
	} else {
		curr = head;
		while (curr->next != NULL) {
			curr = curr->next;
		}
		curr->next = newP;
	}
}
void put_front(pcb_t* newP) {
    pcb_t* curr;
    newP->next = NULL;
    if (head == NULL) {
        head = newP;
    } else {
        curr = head;
        newP->next = curr;
        head = newP;
    }
}

pcb_t* remove_front() {
	pcb_t* curr;
	if (head == NULL) {
		return NULL;
	} else {
	   curr = head;
	   head = head->next;
    }   
	return curr;
}

pcb_t* get_priority() {
    pcb_t* curr;
    if (head == NULL) {
        return NULL;
    }
    pcb_t* highest = head;
    curr = head;
    while (curr != NULL) {
        if (highest->static_priority < curr->static_priority) {
            highest = curr;
        }
        curr = curr->next;
    }
    // curr = head;
    // if (highest == head) {
    //     remove_front();
    //     return highest;
    // }
    // while (curr->next != NULL) {
    //     if (curr->next == highest) {
    //         curr->next = highest->next;
    //         return highest;
    //     }
    // }
    // curr = NULL;
    // return highest;
    // When node to be deleted is head node
    if(head == highest)
    {
        remove_front();
        return highest;
    }
    pcb_t *prev = head;
    while(prev->next != NULL && prev->next != highest) {
        prev = prev->next;
    }
    // Check if node really exists in Linked List
    if(prev->next == NULL)
    {
        return NULL;
    }
    // Remove node from Linked List
    prev->next = prev->next->next;
 
    return highest; 

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
 *	The current array (see above) is how you access the currently running
 *	process indexed by the cpu id. See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information 
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
    pthread_mutex_lock(&ready_mutex);
    pcb_t *newP;
    if (priority) {
        newP = get_priority();
    } else {
        newP = remove_front();
    }
    pthread_mutex_unlock(&ready_mutex);
    if (newP == NULL) {
        context_switch(cpu_id, NULL, time_slice);
    } else {
        pthread_mutex_lock(&ready_mutex);
        newP->state = PROCESS_RUNNING;
        pthread_mutex_unlock(&ready_mutex); 

        pthread_mutex_lock(&current_mutex);
        current[cpu_id] = newP;
        context_switch(cpu_id, newP, time_slice);
        pthread_mutex_unlock(&current_mutex);
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
    /* FIX ME */
    pthread_mutex_lock(&ready_mutex);
    while (head == NULL) {
        pthread_cond_wait(&isEmpty, &ready_mutex);
    }
    pthread_mutex_unlock(&ready_mutex);
    schedule(cpu_id);

    /*
     * REMOVE THE LINE BELOW AFTER IMPLEMENTING IDLE()
     *
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.  Until a ready queue is implemented, we'll put the
     * thread to sleep to keep it from consuming 100% of the CPU time.  Once
     * you implement a proper idle() function using a condition variable,
     * remove the call to mt_safe_usleep() below.
     */
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
    pcb_t* curr;
    pthread_mutex_lock(&current_mutex);
    curr = current[cpu_id];
    curr->state = PROCESS_READY;
    pthread_mutex_unlock(&current_mutex);
    pthread_mutex_lock(&ready_mutex);
    put(curr);
    pthread_mutex_unlock(&ready_mutex);
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
    
    int i;
    int lowest = -1;
    pthread_mutex_lock(&ready_mutex);
    pthread_mutex_lock(&current_mutex);
    if (priority) {
        for (i = 0; i < cpu_count; i++) {
            if (current[i] != NULL) {
                if (current[i]->static_priority < process->static_priority) {
                    if(lowest == -1){
                        // printf("first lowest\n");
                        lowest = i;
                    }
                    if (current[lowest]->static_priority > current[i]->static_priority) {
                        lowest = i;
                        // printf("KKKKKKKKlowest: %d \n", lowest);
                    }                               
                }
            }
        }
        if (lowest != -1) {
            pthread_mutex_unlock(&ready_mutex);
            pthread_mutex_unlock(&current_mutex);
            if(head != NULL) {
                force_preempt(lowest);
            }
            pthread_mutex_lock(&current_mutex);
            pthread_mutex_lock(&ready_mutex);
        } 
    }
    process->state = PROCESS_READY;
    
    put(process);
    pthread_cond_signal(&isEmpty);
    pthread_mutex_unlock(&current_mutex);
    pthread_mutex_unlock(&ready_mutex);
    // printf("find something here"); 
    
}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -p command-line parameters.
 */
int main(int argc, char *argv[])
{

    /* Parse command-line arguments */
    if (argc < 2 || argc > 4 )
    {
        fprintf(stderr, "CS 2200 Project 4 -- Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p ]\n"
            "    Default : FIFO Scheduler\n"
            "         -r : Round-Robin Scheduler\n"
            "         -p : Static Priority Scheduler\n\n");
        return -1;
    }
    cpu_count = atoi(argv[1]);

    /* FIX ME - Add support for -r and -p parameters*/
    if (argc == 2) {
        printf("Running FIFO\n");
    } else {
        if (argv[2][1] == 'r') {
            printf("Running Round-Robin\n");
            time_slice = atoi(argv[3]);
        } else if (argv[2][1] == 'p') {
            printf("Running Priority\n");
            priority = 1;
        }
    }

    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);

    pthread_mutex_init(&ready_mutex, NULL);
    pthread_cond_init(&isEmpty, NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}


