//First Assignment of RTOS, Benedetti Lorenzo

/* 

	Starting from the exercise with 3 tasks scheduled with Rate Monotonic:
	
	These are the requirements in order to fulfill the Assignment given by Professor Sgorbissa:

		- Create an application with 4 periodic tasks, with period 80ms, 100ms, 200ms, 160ms (the task with the highest 			  priority is called Task1, the one with the lowest priority Task4)

		- Create 3 global variables called T1T2, T1T4, T2T3.

		- Task1  shall write something into T1T2, Task 2 shall read from it.

		- Task1  shall write something into T1T4, Task 4 shall read from it.

		- Task2  shall write something into T2T3, Task 3 shall read from it.

		- All critical sections shall be protected by semaphores

		- Semaphores shall use Priority Ceiling 

*/


//------------------------------ LIBRARIES ------------------------------------------------

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>

//--------------------- DEFINING SOME PARAMETERS ----------------------------------------

#define INNERLOOP 100 
#define OUTERLOOP 2000
#define NPERIODICTASKS 4 // number of periodic tasks
#define NAPERIODICTASKS 0 // number of aperiodic tasks
#define NTASKS NPERIODICTASKS + NAPERIODICTASKS

//------- THIS FUNCTION IS USED ONLY TO WASTE TIME DURING THE TASKS' EXECUTION------------

void wastingtime(){

	float wt;
	
	for (int i = 0; i < OUTERLOOP; i++){
 		
 		for (int j = 0; j < INNERLOOP; j++){
 		
			wt = rand() * rand();
		}
	}
}

//---------------------- INIZIALIZE FUNCTIONS OF PERIODIC TASK ----------------------

void Task1();
void Task2();
void Task3();
void Task4();

//------------------- CHARACTERISTIC FUNCTION OF THE THREAD ------------------------

void *task1(void *);
void *task2(void *);
void *task3(void *);
void *task4(void *);

//------------------------- DECLARING VARIABLES USED IN THE CODE -------------------------------------

int maxB1 = 0;
int maxB2 = 0;
int maxB3 = 0;
int maxB4 = 0;

float T1T2 = 0;
float T1T4 = 0;
float T2T3 = 0;

double Ulub;
double U;

// CRITICAL SECTIONS OF THE TASKS
double z11; 
double z12;
double z21;
double z22;
double z31;
double z41;

//--------------------------- DECLARE MUTAXES ----------------------------------------

pthread_mutex_t mutexT1T2;
pthread_mutex_t mutexT1T4;
pthread_mutex_t mutexT2T3;

//---------------------- ARRAYS NEEDED FOR THE CODE ------------------------------

// PERIODS OF THE TASKS
long int periods[NTASKS];

struct timespec next_arrival_time[NTASKS];

double WCET[NTASKS];

pthread_attr_t attributes[NTASKS];

pthread_t thread_id[NTASKS];

struct sched_param parameters[NTASKS];

int missed_deadlines[NTASKS];

// BETAI_star IS THE SET OF CRITICAL SECTIONS THAT BLOCK THE I-TH TASK
double BETA1_star[2];
double BETA2_star[2]; 
double BETA3_star[1]; 
double BETA4_star[0];

//-------------------------------- MAIN ------------------------------------------------

int main(){

  // SETTING TASK PERIODS IN NANOSECONDS
  periods[0] = 80000000;  
  periods[1] = 100000000; 
  periods[2] = 160000000; 
  periods[3] = 200000000;

  //IT'S USEFULL TO ASSIGN A NAME TO THE MAXIMUM AND MINIMUM
  //PRIORITY IN THE SYSTEM
  struct sched_param prio_max;
  prio_max.sched_priority = sched_get_priority_max(SCHED_FIFO);
  
  struct sched_param prio_min;
  prio_min.sched_priority = sched_get_priority_min(SCHED_FIFO);
  
  //PTHREAD MUTEX ATTRIBUTES
  pthread_mutexattr_t mymutexattr;
  pthread_mutexattr_init(&mymutexattr);
  
  //SET PROTOCOL OF MUTEXES TO PRIORITY CEILING
  pthread_mutexattr_setprotocol(&mymutexattr, PTHREAD_PRIO_PROTECT);


  // WE HAVE TO SET THE MAXIMUM PRIORITY TO THE CURRENT THREAD
  // WE HAVE TO CHECK THAT THE MAIN THREAD IS EXECUTED WITH SUPERUSER PRIVILEGES
  // BEFORE ANYTHING ELSE.

  if (getuid() == 0){
  
  	pthread_setschedparam(pthread_self(), SCHED_FIFO, &prio_max);
  }

  // EXECUTE ALL TASKS FOR EACH TIME, FINDING THE WORST
  // CASE EXECUTION TIME FOR EVERY TASK.

  for (int i = 0; i < NTASKS; i++){
  
  	struct timespec time_1; 
  	struct timespec time_2;
	WCET[i] = 0;
	
	for (int j = 0; j < 50; j++){
	
	      clock_gettime(CLOCK_REALTIME, &time_1);
	      
	      if (i == 0){
	      
		Task1();
	      }
	      
	      if (i == 1){
	      
		Task2();
	      }
		
	      if (i == 2){
	      
		Task3();
	      }
	      
	      if (i == 3){
	      
		Task4();
	      }
	      
	      clock_gettime(CLOCK_REALTIME, &time_2);
	      
      	      float RT = 1000000000 * (time_2.tv_sec - time_1.tv_sec) + (time_2.tv_nsec - time_1.tv_nsec);
      	      
      	      if (RT > WCET[i]){
      	      
               WCET[i] = RT;
              }
	}
              printf("\nWorst Case Execution Time %d=%f\n", i, WCET[i]);
  }
  
  // CREATE BETAI_star
  BETA1_star[0] = z21;
  BETA1_star[1] = z41;
  BETA2_star[0] = z31;
  BETA2_star[0] = z41;
  BETA3_star[0] = z41;

  // MAXIMUM BLOCKING TIME
  
  for(int i = 0; i < sizeof(BETA1_star) / sizeof(BETA1_star[0]); i++){
  
    if(BETA1_star[i] > maxB1){
    
    	maxB1 = BETA1_star[i];
    }
  }
 
  for(int i = 0; i < sizeof(BETA2_star) / sizeof(BETA2_star[0]); i++){
  
    if(BETA2_star[i] > maxB2){
    
    	maxB2 = BETA2_star[i];
    }
  }
  
  for(int i = 0; i < sizeof(BETA3_star) / sizeof(BETA3_star[0]); i++){
  
    if(BETA3_star[i] > maxB3){
    
    	maxB3 = BETA3_star[i];
    }
  }
  
  for(int i = 0; i < sizeof(BETA4_star) / sizeof(BETA4_star[0]); i++){
  
    if(BETA4_star[i] > maxB4){
    
    	maxB4 = BETA4_star[i];
    }
  }
  
  // SCHEDULABILITY OF TASKS WITH PRIORITY CEILING
  
  double U_pr_ceil[NTASKS];
  int i;
  
  for (i = 0; i < NTASKS; i++){
  	
  	Ulub = (i + 1.0) * (pow(2.0, (1.0 / (i + 1.0))) - 1.0);
  
  	if (i == 0){
  	
  		U_pr_ceil[i] = (maxB1 / periods[i]);
    	}
    		
  	if (i == 1){
  	
  		U_pr_ceil[i] = (maxB2 / periods[i]); 		
    	}

  	if (i == 2){
  	
  		U_pr_ceil[i] = (maxB3 / periods[i]);
    	}
    	
  	if (i == 3){
  	
  		U_pr_ceil[i] = (maxB4 / periods[i]);
	}
    	
    	int j;
    	for (j = 0; j <= i; j++){
    	
   	   U_pr_ceil[i] = U_pr_ceil[i] + (WCET[j] / periods[j]);
        }

    //CHECKING THE SUFFICIENT CONDITION: EXIT IN CASE ARE NOT SATISFIED
    if (U_pr_ceil[i] > Ulub){
    
      printf("\nU=%lf > Ulub=%lf Non schedulable Task Set\n", U_pr_ceil[i], Ulub);
      return (-1);
    }
    
    printf("\nU(%d)=%lf < %lf=Ulub(%d)\n", i, U_pr_ceil[i], Ulub, i);
  }
  
  printf("\nU=%lf Ulub=%lf Schedulable Task Set\n", U_pr_ceil[3], Ulub);
  fflush(stdout);
  sleep(5);

  // SET THE MINIMUM PRIORITY TO THE CURRENT THREAD: 
  // WE WILL ASSIGN HIGHER PRIORITIES TO THE PERIODIC THREADS WE ARE
  // GOING TO CREATE.

  if (getuid() == 0){
  
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &prio_min);
  }

  // SET ATTRIBUTE OF EACH TASK, INCLUDING SCHEDULING POLICY AND PRIORITY
  for (i = 0; i < NPERIODICTASKS; i++){
  
    // INITIALIZING ATTRIBUTE STRUCTURE OF TASK I
    pthread_attr_init(&(attributes[i]));

    // SET THE ATTRIBUTES TO TELL THE KERNEL THAT THE PRIORITIES AND
    // POLICIES ARE EXPLICITLY CHOSEN
    pthread_attr_setinheritsched(&(attributes[i]), PTHREAD_EXPLICIT_SCHED);

    // SET THE ATTRIBUTES TO SET THE SCHED_FIFO POLICY
    pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);

    // SET THE PARAMETERS TO ASSIGN THE PRIORITY 
    // INVERSELY PROPORTIONAL TO THE PERIOD
    parameters[i].sched_priority = prio_min.sched_priority + NTASKS - i;

    // SET THE ATTRIBUTES AND THE PARAMETERS OF THE CURRENT THREAD
    pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
  }

  // SET THE PRIORITY TO EVERY MUTEX CONSIDERING PRIORITY CEILING PROTOCOL 
  
  // FIRST SEMAPHORE PROTECTS Z_11 AND Z_21
  pthread_mutexattr_setprioceiling(&mymutexattr, parameters[0].sched_priority);
  pthread_mutex_init(&mutexT1T2, &mymutexattr);

  // SECOND SEMAPHORE PROTECTS Z_12 E Z_41
  pthread_mutexattr_setprioceiling(&mymutexattr, parameters[0].sched_priority);
  pthread_mutex_init(&mutexT1T4, &mymutexattr);

  // THIRD SEMAPHORE PROTECTS Z_22 AND Z_31
  pthread_mutexattr_setprioceiling(&mymutexattr, parameters[1].sched_priority);
  pthread_mutex_init(&mutexT2T3, &mymutexattr);

  // DECLARE THE VARIABLE TO CONTAIN THE RETURN VALUES OF PTHREAD_CREATE
  int iret[NTASKS];

  // DECLARE VARIABLES TO READ THE CURRENT TIME
  struct timespec time_1;
  clock_gettime(CLOCK_REALTIME, &time_1);

  // SET THE NEXT ARRIVAL TIME FOR EACH TASK:
  // THIS IS THE END OF THE FIRST PERIOD
  for (i = 0; i < NPERIODICTASKS; i++){
  
    long int next_arrival_nanoseconds = time_1.tv_nsec + periods[i];
    
    next_arrival_time[i].tv_nsec = next_arrival_nanoseconds % 1000000000;
    next_arrival_time[i].tv_sec = time_1.tv_sec + next_arrival_nanoseconds / 1000000000;
    
    missed_deadlines[i] = 0;
  }

  // CREATE THE THREADS 
  iret[0] = pthread_create(&(thread_id[0]), &(attributes[0]), task1, NULL);
  iret[1] = pthread_create(&(thread_id[1]), &(attributes[1]), task2, NULL);
  iret[2] = pthread_create(&(thread_id[2]), &(attributes[2]), task3, NULL);
  iret[3] = pthread_create(&(thread_id[3]), &(attributes[3]), task4, NULL);

  // JOIN ALL THREADS
  pthread_join(thread_id[0], NULL);
  pthread_join(thread_id[1], NULL);
  pthread_join(thread_id[2], NULL);
  pthread_join(thread_id[3], NULL);
  
  // PRINT MISSED DEADLINE TASK
  for (i = 0; i < NTASKS; i++){
  
    printf("\nMissed Deadlines Task %d=%d\n", i, missed_deadlines[i]);
    fflush(stdout);
  }

  exit(0);
}

//-------------------------- APPLICATION OF TASK 1 ------------------------------------------

void Task1(){

  // PRINT ID'S CURRENT TASK
  printf(" 1[ " );
  fflush(stdout);

  struct timespec start1, finish1;
  struct timespec start2, finish2;
  
  // LOCK MUTEX T1T2
  pthread_mutex_lock(&mutexT1T2);
  
  // GET THE CURRENT TIME
  clock_gettime(CLOCK_REALTIME, &start1);
 
  wastingtime();
  
  // WRITING IN T1T2
  T1T2 = rand() % 100 + 1; //RANDOM NUMBER GENERATOR BETWEEN 1 AND 100
  
  //UNLOCK MUTEX
  pthread_mutex_unlock(&mutexT1T2);
  
  // GET THE CURRENT TIME
  clock_gettime(CLOCK_REALTIME, &finish1);
  
  // COMPUTE Z_11
  z11 = 1000000000 * (finish1.tv_sec - start1.tv_sec) + (finish1.tv_nsec - start1.tv_nsec);
  
  // GET THE CURRENT TIME
  clock_gettime(CLOCK_REALTIME, &start2);
  
  // LOCK MUTEX T1T4
  pthread_mutex_lock(&mutexT1T4);
  
  wastingtime();
  
  // WRITING IN T1T4
  T1T4 = rand() % 100 + 1;
  
  // UNLOCK MUTEX T1T4
  pthread_mutex_unlock(&mutexT1T4);
  
  // GET THE CURRENT TIME
  clock_gettime(CLOCK_REALTIME, &finish2);
  
  // COMPUTE Z_12
  z12 = 1000000000 * (finish2.tv_sec - start2.tv_sec) + (finish2.tv_nsec - start2.tv_nsec);
  printf(" ]1 ");
  fflush(stdout);
}

// THREAD CODE FOR TASK 1
void *task1(void *ptr){

  // SET THREAD AFFINITY
  cpu_set_t cset;
  CPU_ZERO(&cset);
  CPU_SET(0, &cset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

  int i;
  for (i = 0; i < 100; i++)
  {
    // EXECUTE APPLICATION CODE 
    
    Task1();

    // CHECKING IF A DEADLINE HAS BEEN MISSED
    struct timespec ct;//ct = current time
    
    clock_gettime(CLOCK_REALTIME, &ct);
    
    double current_time = 1000000000 * (ct.tv_sec) + ct.tv_nsec;
    double Next_Arr_Time = 1000000000 * (next_arrival_time[0].tv_sec) + next_arrival_time[0].tv_nsec;
    
    // A MISSED DEADLINE WILL OCCUR IF CT IS GREATER THAN NEXT ARRIVAL TIME
    if (current_time > Next_Arr_Time){
    
      missed_deadlines[0] = missed_deadlines[0] + 1;
    }

    // SLEEPING UNTIL THE END OF THE CURRENT PERIOD (START OF NEW PERIOD)
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[0], NULL);

    // THE THREAD IS READY
    long int next_arrival_nanoseconds = next_arrival_time[0].tv_nsec + periods[0];
    next_arrival_time[0].tv_nsec = next_arrival_nanoseconds % 1000000000;
    next_arrival_time[0].tv_sec = next_arrival_time[0].tv_sec + next_arrival_nanoseconds / 1000000000;
  }
}

//-------------------------- APPLICATION OF TASK 2 ------------------------------------------

void Task2(){

  float ReadingT1T2;
  
  // PRINT ID'S CURRENT TASK
  printf(" 2[ ");
  fflush(stdout);

  struct timespec start3, finish3;
  struct timespec start4, finish4;
  
  // GET THE CURRENT 
  clock_gettime(CLOCK_REALTIME, &start3);
  
  // LOCK MUTEX T1T2
  pthread_mutex_lock(&mutexT1T2);
  
  wastingtime();
  
  // READING IN T1T2
  ReadingT1T2 = T1T2;

  // UNLOCK T1T2
  pthread_mutex_unlock(&mutexT1T2);
  
  // GET THE CURRENT TIME
  clock_gettime(CLOCK_REALTIME, &finish3);
  
  // COMPUTE Z_21
  z21 = 1000000000 * (finish3.tv_sec - start3.tv_sec) + (finish3.tv_nsec - start3.tv_nsec);
  
  // GET THE CURRENT TIME
  clock_gettime(CLOCK_REALTIME, &start4);
  
  // LOCK MUTEX T2T3
  pthread_mutex_lock(&mutexT2T3);
  
  wastingtime();
  
  // WRITING IN T2T3
  T2T3 = rand() % 100 + 1;
  
  // UNLOCK MUTEX T2T3
  pthread_mutex_unlock(&mutexT2T3);
  
  // GET THE CURRENT TIME 
  clock_gettime(CLOCK_REALTIME, &finish4);
  
  //COMPUTE Z22
  z22 = 1000000000 * (finish4.tv_sec - start4.tv_sec) + (finish4.tv_nsec - start4.tv_nsec);
  
  // PRINT ID'S CURRENT TASK
  printf(" ]2 ");
  fflush(stdout);
}

void *task2(void *ptr){

  // SET THREAD AFFINITY
  cpu_set_t cset;
  CPU_ZERO(&cset);
  CPU_SET(0, &cset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

  int i;
  for (i = 0; i < 100; i++){
  
    Task2();

    // CHECKING IF A DEADLINE HAS BEEN MISSED
    struct timespec ct;
    
    clock_gettime(CLOCK_REALTIME, &ct);
    
    double current_time = 1000000000 * (ct.tv_sec) + ct.tv_nsec;
    double Next_Arr_Time = 1000000000 * (next_arrival_time[1].tv_sec) + next_arrival_time[1].tv_nsec;
    
    // A MISSED DEADLINE WILL OCCUR IF CT IS GREATER THAN NEXT ARRIVAL TIME
    if (current_time > Next_Arr_Time)
    {
      missed_deadlines[1] = missed_deadlines[1] + 1;
    }

    //SAME THINGS AS BEFORE...
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[1], NULL);
    long int next_arrival_nanoseconds = next_arrival_time[1].tv_nsec + periods[1];
    
    next_arrival_time[1].tv_nsec = next_arrival_nanoseconds % 1000000000;
    next_arrival_time[1].tv_sec = next_arrival_time[1].tv_sec + next_arrival_nanoseconds / 1000000000;
  }
}

//-------------------------- APPLICATION OF TASK 3 ---------------------------------------------

void Task3(){

  float ReadingT2T3;

  // PRINT ID'S CURRENT TASK
  printf(" 3[ ");
  fflush(stdout);
  
  struct timespec start5, finish5;
  
  // GET THE CURRENT TIME
  clock_gettime(CLOCK_REALTIME, &start5);
  
  // LOCK MUTEX T2T3
  pthread_mutex_lock(&mutexT2T3);
  
  wastingtime();
  
  // READING IN T2T3
  ReadingT2T3 = T2T3;
  
  // UNLOCK MUTEX
  
  pthread_mutex_unlock(&mutexT2T3);
  
  // GET THE CURRENT
  clock_gettime(CLOCK_REALTIME, &finish5);
  
  // COMPUTE Z31
  z31 = 1000000000 * (finish5.tv_sec - start5.tv_sec) + (finish5.tv_nsec - start5.tv_nsec);
  
  // PRINT ID'S CURRENT TASK
  printf(" ]3 ");
  fflush(stdout);
}

void *task3(void *ptr){

  // SET THREAD AFFINITY
  cpu_set_t cset;
  CPU_ZERO(&cset);
  CPU_SET(0, &cset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

  int i;
  for (i = 0; i < 100; i++){
  
    Task3();
    
    // CHECKING IF A DEADLINE HAS BEEN MISSED
    struct timespec ct;
    
    clock_gettime(CLOCK_REALTIME, &ct);
    
    double current_time = 1000000000 * (ct.tv_sec) + ct.tv_nsec;
    double Next_Arr_Time = 1000000000 * (next_arrival_time[2].tv_sec) + next_arrival_time[2].tv_nsec;
    
    if (current_time > Next_Arr_Time){
    
      missed_deadlines[2] = missed_deadlines[2] + 1;
    }

    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[2], NULL);
    
    long int next_arrival_nanoseconds = next_arrival_time[2].tv_nsec + periods[2];
    
    next_arrival_time[2].tv_nsec = next_arrival_nanoseconds % 1000000000;
    next_arrival_time[2].tv_sec = next_arrival_time[2].tv_sec + next_arrival_nanoseconds / 1000000000;
  }
}

//-------------------------- APPLICATION OF TASK 4 ---------------------------------------------

void Task4()
{

  float ReadingT1T4;
  
  // PRINT ID'S CURRENT TASK
  printf(" 4[ ");
  fflush(stdout);
  
  
  struct timespec start6, finish6;
  
  // GET THE CURRENT TIME
  clock_gettime(CLOCK_REALTIME, &start6);
  
  // LOCK MUTEX T1T4
  pthread_mutex_lock(&mutexT1T4);
  
  wastingtime();
  
  // READING IN T1T4
  ReadingT1T4 = T1T4; 

  // UNLOCK MUTEX
  pthread_mutex_unlock(&mutexT1T4);
  
  // GET THE CURRENT TIME
  clock_gettime(CLOCK_REALTIME, &finish6);
  
  // COMPUTE Z41
  z41 = 1000000000 * (finish6.tv_sec - start6.tv_sec) + (finish6.tv_nsec - start6.tv_nsec);
  
  // PRINT ID'S CURRENT TASK
  printf(" ]4 ");
  fflush(stdout);
}

void *task4(void *ptr)
{
  // SET THREAD AFFINITY
  cpu_set_t cset;
  CPU_ZERO(&cset);
  CPU_SET(0, &cset);
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cset);

  int i;
  for (i = 0; i < 100; i++){
  
    Task4();

    // CHECKING IF A DEADLINE HAS BEEN MISSED
    struct timespec ct;

    clock_gettime(CLOCK_REALTIME, &ct);
    
    double current_time = 1000000000 * (ct.tv_sec) + ct.tv_nsec;
    double Next_Arr_Time = 1000000000 * (next_arrival_time[3].tv_sec) + next_arrival_time[3].tv_nsec;
    
    if (current_time > Next_Arr_Time)
    {
      missed_deadlines[3] = missed_deadlines[3] + 1;
    }

    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_arrival_time[3], NULL);
    
    long int next_arrival_nanoseconds = next_arrival_time[3].tv_nsec + periods[3];
    next_arrival_time[3].tv_nsec = next_arrival_nanoseconds % 1000000000;
    next_arrival_time[3].tv_sec = next_arrival_time[3].tv_sec + next_arrival_nanoseconds / 1000000000;
  }
}


