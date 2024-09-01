#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define EVENTCOUNT 10
#define CAPACITY 500
#define WORKERCOUNT 5
#define MINTICKETS 5
#define MAXTICKETS 10
#define MAX 5
#define SLEEPT 60
#define DELAY 3
#define SLEEPTIME (rand()%DELAY+1)
#define SLEEPIDLE (rand()%DELAY+1)

int kill_count, load_count;
pthread_mutex_t kill_mutex,wait_mutex,table_mutex;
pthread_cond_t waiting_queries;

struct sharedTable{
	int eventNumber;
	int queryType;
	long ID;
};
typedef struct sharedTable sharedTable;

sharedTable ActiveQueries[MAX];
int reservationStatus[EVENTCOUNT];


int isQueryRunningForSameEvent(int eventNumber){
	for(int i=0;i<MAX;i++){
		if(ActiveQueries[i].eventNumber == eventNumber && (ActiveQueries[i].queryType == 2 || ActiveQueries[i].queryType == 3)){
			return 1;
		}
	}
	return 0;
}

int getSlotInTable(int queryType,int eventNumber,long ID){
	int id = -1;
	for(int i =0;i<MAX;i++){
		if(ActiveQueries[i].eventNumber == -1){
			id = i;
			break;
		}
	}
	if(id== -1 || id<0 || id >= MAX){
		printf("\n******* No empty slot in table!*********\n");
	}
	if(isQueryRunningForSameEvent(eventNumber) == 1){
		return -1;
	}
	ActiveQueries[id].eventNumber = eventNumber;
	ActiveQueries[id].queryType = queryType;
	ActiveQueries[id].ID = ID;
	return id;
}

void getAvailableSeats(int eventNumber){

	if(eventNumber<0 || eventNumber>=EVENTCOUNT){
		printf("\n*** Invalid query = %d ***\n",eventNumber);
	}
	int rem_seats = CAPACITY - reservationStatus[eventNumber];
	printf("\nNo. of available seats for event %d = %d\n", eventNumber, rem_seats);
}

void bookTickets(int eventNumber,int ticketCount,int *bookedHistory){

	if(eventNumber<0 || eventNumber>=EVENTCOUNT){
		printf("\n*** Invalid query = %d ***\n",eventNumber);
	}

	int rem_seats = CAPACITY - reservationStatus[eventNumber];
	if(ticketCount > rem_seats){
		printf("\nFAILURE : No. of bookings in query = %d is more than %d seats available for event %d", ticketCount, rem_seats, eventNumber);
		printf("\nCannot complete the execution of query!\n");
		return;
	}

	printf("\nSUCCESS : %d tickets successfully booked for event %d\n",ticketCount,eventNumber);
	bookedHistory[eventNumber] = bookedHistory[eventNumber] + ticketCount;
	reservationStatus[eventNumber] = reservationStatus[eventNumber] + ticketCount;	
}

void cancelBookedTicket(int eventNumber,int *bookingHistory){

	if(eventNumber<0 || eventNumber>=EVENTCOUNT){
		printf("\n*** Invalid query = %d ***\n",eventNumber);
	}
	
	if(reservationStatus[eventNumber] == 0){
		printf("\nERROR : No booked tickets for event %d\n",eventNumber);
		return;
	}

	reservationStatus[eventNumber]--;
	bookingHistory[eventNumber]++;
	printf("\nTicket cancelled for event %d\n",eventNumber);
}

void getRandomQuery(int *queryType,int *eventNumber,int *ticketCount,int *bookingHistory){

	*eventNumber = rand()%EVENTCOUNT;

	switch(rand()%3+1){
		case 1:
			*queryType = 1;
			*ticketCount = -1;
			break;
		case 2:
			*queryType = 2;
			*ticketCount = rand()%(MAXTICKETS - MINTICKETS + 1) + MINTICKETS;
			break;
		case 3:
			*queryType = 3;
			if(bookingHistory[*eventNumber]<=0){
				*queryType = 1;
				break;
			}
			*ticketCount = -1;
			break;
	}
	return;
}

void executeQuery(int queryType,int eventNumber,int ticketCount,int *bookedHistory,long ID){

	pthread_mutex_lock(&table_mutex);
	int id = getSlotInTable(queryType,eventNumber,ID);
	pthread_mutex_unlock(&table_mutex);

	if(id == -1){
		return;
	}
	
	switch(queryType){
		case 1:
			printf("\nThread %ld : Query type = %d, Event number = %d", ID, queryType, eventNumber);
			getAvailableSeats(eventNumber);
			break;
		case 2:
			printf("\nThread %ld : Query type = %d, Event number = %d, Ticket count = %d", ID, queryType, eventNumber, ticketCount);
			bookTickets(eventNumber,ticketCount,bookedHistory);
			break;
		case 3:
			printf("\nThread %ld : Query type = %d, Event number = %d", ID, queryType, eventNumber);
			cancelBookedTicket(eventNumber,bookedHistory);
			break;
		default:
			printf("\nERROR : Unknown query of type %d", queryType);
			break;
	}
	pthread_mutex_lock(&table_mutex);
	
	if(id < 0 || id>=MAX){
		printf("\nERROR : No entry with index %d in active queries table",id);
		exit(0);
	}
	ActiveQueries[id].eventNumber = -1;
	pthread_mutex_unlock(&table_mutex);
}

void *doWork(void *t){
	long ID = (long)(t);
	printf("\nWORKER THREAD %ld STARTS\n",ID);
	int queryType,eventNumber,k=-1;
	int bookingHistory[EVENTCOUNT];
	for(int i=0;i<EVENTCOUNT;i++){
		bookingHistory[i] = 0;
	}
	while(1){
	
		int temp;
		pthread_mutex_lock(&kill_mutex);
		temp = kill_count;
		pthread_mutex_unlock(&kill_mutex);
		if(temp == 1){
			break;
		}
		
		// for making sure that the system is not overloaded
		pthread_mutex_lock(&wait_mutex);
		load_count++;
		
		if(load_count>MAX){
			pthread_cond_wait(&waiting_queries, &wait_mutex);
		}
		
		pthread_mutex_unlock(&wait_mutex);
		
		printf("\n*** WORKER THREAD %ld acquired access ***\n",ID);

		getRandomQuery(&queryType,&eventNumber,&k,bookingHistory);
		
		// for simulating time to complete an operation
		sleep(SLEEPTIME);
		
		// executing the query
		executeQuery(queryType,eventNumber,k,bookingHistory,ID);

		printf("\n*** WORKER THREAD %ld released access ***\n",ID);
		pthread_mutex_lock(&wait_mutex);
		load_count--;
		pthread_cond_signal(&waiting_queries);
		pthread_mutex_unlock(&wait_mutex);

		// idle time
		sleep(SLEEPIDLE);
	}
	printf("\n*** WORKER THREAD %ld ENDS ***\n",ID);
	pthread_exit(NULL);
}

int main(){
	system("reset");
	srand(time(0)); 
	
	// initialising mutexes
	pthread_mutex_init(&kill_mutex, NULL);
	pthread_mutex_trylock(&kill_mutex);
	pthread_mutex_unlock(&kill_mutex);
	
	pthread_mutex_init(&wait_mutex, NULL);
	pthread_mutex_trylock(&wait_mutex);
	pthread_mutex_unlock(&wait_mutex);
	
	pthread_mutex_init(&table_mutex, NULL);
	pthread_mutex_trylock(&table_mutex);
	pthread_mutex_unlock(&table_mutex);
	
	pthread_cond_init(&waiting_queries, NULL);
	
	kill_count = 0;
	load_count = 0;
	
	for(int i=0;i<EVENTCOUNT;i++){
		reservationStatus[i] = 0;
	}
	
	// setting values for blank entries in the active queries table
	for(int i=0;i<MAX;i++){
		ActiveQueries[i].eventNumber = -1;
		ActiveQueries[i].queryType = -1;
		ActiveQueries[i].ID = 0;
	}
	
	pthread_t threads[WORKERCOUNT];
	
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	for(int i = 0;i<WORKERCOUNT;i++){
		long ID = i+1;
		if(pthread_create(threads + i, &attr, doWork, (void *)ID)){
			fprintf(stderr, "Master thread : Error creating worker thread\n");
			pthread_attr_destroy(&attr);
			exit(1);
    	}
	}
	pthread_attr_destroy(&attr);
	
	
	sleep(SLEEPT);
	
	pthread_mutex_lock(&kill_mutex);
	kill_count = 1;
	pthread_mutex_unlock(&kill_mutex);
	
	// Waiting for workers to complete
	for(int i=0; i<WORKERCOUNT; i++){
		pthread_join(threads[i], NULL);
	}
	printf("Master thread : Waited for %d workers.\n", WORKERCOUNT);
	
	// Free all mutexes
	pthread_mutex_destroy(&kill_mutex);
	pthread_mutex_destroy(&wait_mutex);
	pthread_mutex_destroy(&table_mutex);
	pthread_cond_destroy(&waiting_queries);
	
	printf("\n*************** RESERVATION STATUS *****************\n");
	printf("\nEvent no.\tNo. of reserved tickets");
	
	for(int i=0;i<EVENTCOUNT;i++){
		printf("\n%d\t\t%d",i,reservationStatus[i]);
	}
	printf("\n");
	
	pthread_exit(NULL);
}
