/*
 * pcb.h
 *
 *  Created on: May 5, 2021
 *      Author: OS1
 */

#ifndef PCB_H_
#define PCB_H_

#define lock asm{pushf; cli;}
#define unlock asm popf;
#define lockF lockFlag--;
#define unlockF lockFlag++;

#define FORCE_QUIT asm { MOV AH, 4CH; MOV AL, 01; INT 21H }
#define AUTO_DELETE_FORK_CHILDREN 0
#define FORK_INIT_SEMAPHORE_LOCAL 1
#define FORK_COPY_TIMESLICE 1

typedef unsigned long StackSize;
typedef unsigned int Time; 				// time, x 55ms
typedef int ID;

extern volatile int lockFlag;

class Thread;
class Queue;

class PCB {
public:

	~PCB();

	struct Node {
		PCB *data;
		Node *next;

		Node() {next = 0; data=0;}
		Node(PCB* d, Node* n) {data=d; next=n;}

		static void add(PCB*);
		static void rem(PCB*);
		static int allFinished();

		static volatile int numActive;
	};
	static volatile Node* root;

	// inicijalizuje sve promenjive PCB strukture
	PCB(StackSize stackSize, Time timeSlice, Thread *t);

	// kreira kontekst prvi put
	unsigned createContext();

	// referenca na PCB koji se trenutno izvrsava i kernel PCB
	static volatile PCB* running;
	static volatile PCB* kernelPCB;

	static void wrapper();
	static void quit();
	static int fixSemaphores(PCB* ol, PCB* nw);

	StackSize stackSize;
	Time timeSlice, origTimeSlice;
	volatile unsigned long semTick;
	volatile unsigned semSig;

	unsigned sp, ss;
	unsigned *stack;
	Thread *th;

	unsigned char finished, dontSchedule, forked;

	// dodaj Queue za Thread::waitToComplete
	Queue* q;
	void freeQueue();

};
class KernelSem;
struct NodeSem {
	KernelSem*data;
	NodeSem*next;
	NodeSem(KernelSem*d,NodeSem*n) : data(d), next(n) {}
};

#endif /* PCB_H_ */
