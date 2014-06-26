/*
 * student.c
 * Multithreaded OS Simulation for CS 2200, Project 4
 *
 * This file contains the CPU scheduler for the simulation.  
 */

 /*
 Kelley Sheffield
 GTID 902601249
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os-sim.h"


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
pcb_t *head;
pcb_t *tail;
pcb_t *temp;

static pthread_mutex_t current_mutex;
static pthread_mutex_t ready_mutex; 
pthread_cond_t stopIdle;  

int roundRobin;
int timeslice;
int preemptive;
int isIdle[16];

static int runTime;
int numCpus;








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
    /* FIX ME */   
    if(roundRobin==1)
    {
        runTime = timeslice;
        
    }
    else
    {
        runTime = -1;
    }
    if(head==NULL)
    {
       context_switch(cpu_id, NULL, runTime); 
    }
    
    else
    {
      pthread_mutex_lock(&ready_mutex);
      temp = head;  
      temp->state = PROCESS_RUNNING;
        
      if(head==tail)
      {
        head = NULL;
        tail = NULL;
      }
      else
      {
        head = temp->next;
      }
      pthread_mutex_lock(&current_mutex);
      current[cpu_id] = temp;
      pthread_mutex_unlock(&current_mutex);
    
     
    
      pthread_mutex_unlock(&ready_mutex); 
   
      context_switch(cpu_id, temp,runTime);
      
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

        pthread_mutex_lock(&current_mutex);
        isIdle[cpu_id] = 0; 
        while(pthread_cond_wait(&stopIdle, &current_mutex));
       
        
        pthread_mutex_unlock(&current_mutex);
        schedule(cpu_id);
        isIdle[cpu_id] = 1;
       
      
    
    
    /*
     * REMOVE THE LINE BELOW AFTER IMPLEMENTING IDLE()
     *
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.  Until a ready queue is implemented, we'll put the
     * thread to sleep to keep it from consuming 100% of the CPU time.  Once
     * you implement a proper idle() function using a condition variable,
     * remove the call to mt_safe_usleep() below.
     */
 /*   mt_safe_usleep(1000000);*/
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
    /* FIX ME */
    pthread_mutex_lock(&current_mutex);
    pthread_mutex_lock(&ready_mutex);
    current[cpu_id]->state = PROCESS_READY;
  if(head==NULL)
   {
     
    
    head = current[cpu_id];
   
    tail = current[cpu_id];
    
    
   }
   else
   {
   
    tail->next = current[cpu_id];
    tail = current[cpu_id];
   }
   
   pthread_mutex_unlock(&ready_mutex);
   pthread_mutex_unlock(&current_mutex);


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
    /* FIX ME */
   
    pthread_mutex_lock(&current_mutex);
    
    if(current[cpu_id]!=NULL)
    {    
     current[cpu_id]->state = PROCESS_WAITING;
    }
     
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

    /* FIX ME */
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
    /* FIX ME */
   process->state = PROCESS_READY;
   
   pthread_mutex_lock(&ready_mutex);


   if(head==NULL)
   {
     
    head = process;
    tail = process;
    
   }
   else
   {

    tail->next = process;
    tail = process;
   }

    pthread_cond_signal(&stopIdle);
    pthread_mutex_unlock(&ready_mutex);    
    int i;
    if(preemptive == 1)
    {
        for(i = 0; i < numCpus+1; i++)
        {
            if((current[i]!=NULL) && (current[i]->static_priority < process->static_priority) && (isIdle[i] == 1))
            {               
                force_preempt(i);
            }        
            
        }
    }   
    
}



/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -p command-line parameters.
 */
int main(int argc, char *argv[])
{
    int cpu_count;
    int i;
    for(i = 0; i < 16; i++)
    {
        isIdle[i] = 0;
    }
    
/*
    currentNode = head;*/

    /* Parse command-line arguments */
    if (argc < 2)
    {
        fprintf(stderr, "CS 2200 Project 4 -- Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p ]\n"
            "    Default : FIFO Scheduler\n"
            "         -r : Round-Robin Scheduler\n"
            "         -p : Static Priority Scheduler\n\n");
        return -1;
    }
    cpu_count = atoi(argv[1]);
    numCpus = cpu_count;

    /* FIX ME - Add support for -r and -p parameters*/
    if(argc > 2)
    {    
        if(strcmp(argv[2],"-r")==0)
        {
            roundRobin = 1;
            timeslice = atoi(argv[3]);
                
        }
        if(strcmp(argv[2],"-p")==0)
        {
            preemptive = 1;            
        }
    }

    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);
    pthread_mutex_init(&ready_mutex, NULL); 

    /* Start the simulator in the library */
    start_simulator(cpu_count);
    

    return 0;
}


