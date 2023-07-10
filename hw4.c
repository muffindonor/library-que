#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <stdlib.h>
#define N 10
#define K 5

typedef struct node {
	int val;
	struct node* next;
}*Node;


Node librarianQue = NULL; //Que for a librarian
Node couchQue = NULL;	  //Que for available couch
Node entryQue = NULL;	  //Que in case library is full

//declare functions
Node addToQue(Node head, int num);
Node removeFromQue(Node head);
void* readerFunction(void* ptr);
void* librarianFunction(void* ptr);
int checkQue(Node que);


//mutexCouch init value 1, allows only one thread to pass through first half of readerFunction
//mutexLibrarian init value 1, allows only one thread to pass through second half of readerFunction
//librarianReady init value 3, indicates if any workers are available
//wakeLibrarian init value 1, signals to librarian thread to wake up
//queMutex init value 1, allows only one thread to modify que
//nextReader init value 0, signals next reader to approach librarian
sem_t mutexCouch, mutexLibrarian, wakeLibrarian, queMutex, nextReader;

//libraryCounter to maintain order
//couchCounter to keep track of emtpy couches
int libraryCounter = 0, couchCounter = K;

int main(int argc, char* argv[]) {
	pthread_t readers[N + 2], librarians[3];
	sem_init(&mutexCouch, 0, 1);
	sem_init(&mutexLibrarian, 0, 3);
	sem_init(&wakeLibrarian, 0, 0);
	sem_init(&queMutex, 0, 1);
	sem_init(&nextReader, 0, 0);
	int  i = 0, number[N+2], test;
	//initialize index array
	for (i = 0; i < N + 2; i++) {
		number[i] = i;
	}
	//Create Librarian threads
	for (i = 0; i < 3; i++) {
		test = pthread_create(&librarians[i], NULL, librarianFunction, (void*)&number[i]);
		if (test < 0) {
			perror("Librarian Thread");
		}
	}
	//Create Reader threads
	for (i = 0; i < N + 2; i++) {
		test = pthread_create(&readers[i], NULL, readerFunction, (void*)&number[i]);
		if (test < 0) {
			perror("Visitor Thread");
		}
	}
	
	//join function to wait for threads
	for (i = 0; i < 3; i++) {
		pthread_join(librarians[i], NULL);
	}
	for (i = 0; i < N+2; i++) {
		pthread_join(readers[i], NULL);
	}
}

void* readerFunction(void* ptr) {
	//Save thread's ID
	int readerId = *(int*)ptr;
	int entryFlag = 0;
	int couchFlag = 0;
	int librarianFlag = 0;
	

	//mutexCouch is hall pass for thread until they find their room on a couch
	//mutexLibrarian is for thread that engages with librarian, released after waking librarian up
	//If thread has no room in library/on couch or no available librarian, thread joins relevant que.
	//Ques are FIFO, thread exits que only when his readerId is first in line.

	while (1) {
		sem_wait(&mutexCouch);
		libraryCounter++;
		if (libraryCounter > 10) {
			printf("I'm Reader #%d, I'm waiting outside! \n", readerId);
			sem_wait(&queMutex);
			entryQue = addToQue(entryQue, readerId);
			sem_post(&queMutex);
			sem_post(&mutexCouch);
			while (entryFlag = 0) {
				if (checkQue(entryQue) == readerId) {
					sem_wait(&mutexCouch);
					entryFlag = 1;
					sem_wait(&queMutex);
					entryQue = removeFromQue(entryQue);
					sem_post(&queMutex);
				}
				else {
					sleep(1);
				}
			}
		}
		printf("I'm Reader #%d, I've entered the library! \n", readerId);

		if (couchCounter == 0) {
			printf("I'm Reader #%d, I'm using the computer until someone gets off the couch \n", readerId);
			sem_wait(&queMutex);
			couchQue = addToQue(couchQue,readerId);
			sem_post(&queMutex);
			sem_post(&mutexCouch);
			while (couchFlag = 0) {
				if (checkQue(couchQue) == readerId) {
					sem_wait(&mutexCouch);
					sem_wait(&queMutex);
					librarianQue = addToQue(librarianQue, readerId);
					couchQue = removeFromQue(couchQue);
					sem_post(&queMutex);
					couchCounter--;
					couchFlag = 1;
				}
				else{
					sleep(1);
				}
			}
		}
		if (couchCounter > 0) {
			printf("I'm Reader #%d, I'm reading on the couch waiting for my turn \n", readerId);
			if (couchFlag != 1) {
				couchCounter--;
				sem_wait(&queMutex);
				librarianQue = addToQue(librarianQue, readerId);
				sem_post(&queMutex);
			}
		}
		sem_post(&mutexCouch);

		while (librarianFlag == 0) { 
			if (checkQue(librarianQue) == readerId) {
				sem_wait(&mutexLibrarian);
				librarianFlag = 1;
			}
			if (librarianFlag == 0) {
				sleep(1);
			}
		}
		//reset thread values
		couchFlag = 0;
		librarianFlag = 0;
		
		couchCounter++;
		libraryCounter--;

		printf("I'm Reader #%d, It's finally my turn to talk with the librarian!! \n", readerId);
		sem_post(&wakeLibrarian);
		sem_wait(&queMutex);
		librarianQue = removeFromQue(librarianQue);
		sem_post(&queMutex);
		sem_wait(&nextReader);
		sem_post(&mutexLibrarian);
		sleep(1);
	}
}

void* librarianFunction(void* ptr) {
	int librarianId;
	librarianId = *(int*)ptr;
	while (1) {
		sem_wait(&wakeLibrarian);
		printf("I'm Librarian #%d, I'm working now\n", librarianId);
		sem_post(&nextReader);
		sleep(1);
	}

}
//return first value in given que
int checkQue(Node que) {
	return que->val;
}

//add reader to FIFO que
Node addToQue(Node head, int num) {
	Node newNode = (Node)malloc(sizeof(struct node));
	if (!newNode) {
		exit(1);
	}
	newNode->val = num;
	newNode->next = NULL;
	if (head == NULL) {
		return newNode;
	}
	else {
		Node current = head;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = newNode;
		return head;
	}
}

//removes first node from que
Node removeFromQue(Node head) {
	if (head == NULL) {
		printf("HEAD IS NULL\n");
		return NULL;
	}
	Node newHead = head->next;
	free(head);
	return newHead;
}





