/*
 * pcb.cpp
 *
 *  Created on: May 5, 2021
 *      Author: OS1
 */
#include <DOS.H>
#include <IOSTREAM.H>
#include "pcb.h"
#include "thread.h"
#include "queue.h"
#include "SCHEDULE.H"
#include <STDIO.H>
#include "semaphor.h"
#include "ksem.h"


/*
 * Cuvam listu PCB objekata a ne Thread-ova (u PCB-u imam referencu na Thread)
 */
void PCB::Node::add(PCB* x) {
	lock
	lockF
	if (PCB::root == 0) {
		PCB::root = new Node(x, 0);
	}
	else {
		volatile PCB::Node *tmp = root;
		while (tmp->next) tmp = tmp->next;
		tmp->next = new Node(x, 0);
	}
	unlock
	unlockF
}

/*
 * Brisem PCB iz list
 */
void PCB::Node::rem(PCB* x) {
	lock
	lockF
	if (PCB::root != 0) {
		if (PCB::root->data == x)
			PCB::root = PCB::root->next;
		else {
			volatile PCB::Node *tmp = PCB::root;
			while (tmp->next) {
				if (tmp->next->data == x) {
					PCB::Node *old = tmp->next;

					tmp->next = tmp->next->next;
					delete old;
				}
				else tmp = tmp->next;
			}
		}
	}
	unlock
	unlockF
}

/*
 * Optimizacija u odnosu na prolazak kroz listu
 */
int PCB::Node::allFinished() {
	return numActive == 0;
}

/*
 * Oslobodi sve niti koje cekaju da se ovaj PCB zavrsi
 */
void PCB::freeQueue() {
	while (1) {
		if (q->size() == 0) break;
		PCB* nova = q->pop();
		Scheduler::put(nova);
		nova->dontSchedule = 0;
	}
}

/*
 * inicijalizacija PCB strukture
 */
PCB::PCB(StackSize stackSize, Time timeSlice, Thread * t) {
	lock
	lockF
	this->stackSize = stackSize/2 + stackSize%2; // jer mi je stack unsigned
	this->timeSlice = timeSlice;
	this->origTimeSlice = timeSlice;
	this->th = t;
	this->finished = dontSchedule = 0;
	this->semTick = 0;
	this->q = new Queue();
	this->forked = 0;

	// pri inicijalizaciji, markiraj stack na 0 zbog destrukora
	this->stack = 0;

	// dodaj u listu
	PCB::Node::add(this);
	unlock
	unlockF
}

/*
 * Kada brisem pcb objekat, izbrisem ga iz liste iz dealociram stack
 * -> Da li je pcb izbacen iz schedulera ? -> to se obezbedjuje iz thread-a, cekajuci da nit zavrsi
 * -> Da li je parent atribut u thread-u odzran ? -> to thread kontrolise
 * GARANTUJEM: Nit se zavrsila pre poziva destruktora. (thread ce da obezbedi)
 */
PCB::~PCB() {
	lock
	lockF
	PCB::Node::rem(this);
	if (this->stack)
		delete this->stack;
	if (this->q != 0)
		delete this->q;
	unlock
	unlockF
}

/*
 * Polazna funkcija niti koja omogucava polimorfno izvrsavanje funkcije run
 */
void PCB::wrapper() {
	running->th->run();
	PCB::quit();
}

/*
 * Funkcija koja fleguje trenutnu nit (pozivajucu) kao zavrsenu
 */
void PCB::quit() {
	lock
	// podesi kraj niti kako se ne bi dodavala u Scheduler
	running->finished = 1;
	PCB::Node::numActive --;
	running->freeQueue();


	unlock

	dispatch();
}

/*
 * Pravi kontekst u vodu alokacije stack-a i inicijalizacije za povratak u funkciju wrapper
 * povratna vrednost predstavlja uspesnost kreiranja steka (alokacija failed ili nije)
 */
unsigned PCB::createContext() {
	lock
	lockF
	stack = new unsigned[stackSize];
	if (stack != 0) {
		stack[stackSize-1] = FP_SEG(PCB::wrapper);
		stack[stackSize-2] = FP_OFF(PCB::wrapper);
		stack[stackSize-3] = 0;							// granica za BP u fork-u

		int need = 	2 +	// SEG i OFFSET funkcije (2x2 bajta)
					4 + // 4 registra opste namene (AX, BX, CX, DX)
					3 + // 3 registra za segment i indeksiranje (ES, SI, DI)
					1 + // 1 mesto za PSW za unlock
					2;  // BP i DS (u funkciji dispatch)

		stack[stackSize-7] = 0x200;

		ss = FP_SEG(stack+stackSize-need);
		sp = FP_OFF(stack+stackSize-need);

		finished = 0;
	}
	unsigned ret = (stack ==0);
	unlock
	unlockF
	return ret;

}

unsigned long truePtr(void *x) {
	long res = FP_SEG(x);
	res <<= 4;
	res += FP_OFF(x);
	return res;
}

extern NodeSem*rootSem;						//
extern void registerSem(KernelSem* sem); 	// iz kernel.cpp

int PCB::fixSemaphores(PCB* ol, PCB* nw) {
	int ret = 1;
	int sdif = (nw->stackSize - ol->stackSize)*sizeof(unsigned);
	for (NodeSem* cur = rootSem; cur !=0; cur = cur->next) {
		Semaphore *sem = cur->data->s;
		if (truePtr(sem) >= truePtr(ol->stack) && truePtr(sem) < truePtr(ol->stack+ol->stackSize)) {
			int offs = FP_OFF(sem)+sdif;
			Semaphore *newSem = (Semaphore*)( MK_FP(nw->ss, offs) );
			lock
			lockF
			newSem->sem = new KernelSem(sem->val(), newSem);
			if (newSem->sem == 0) ret = 0;
			if (newSem->sem != 0 && (newSem->sem->blocked == 0 || newSem->sem->blockedWait == 0)) {newSem->sem->val=0;delete newSem->sem;ret=0;}//while(1) cout << "FATAL ERROR" << endl;}
			unlock
			unlockF
		}
	}
	return ret;
}

volatile PCB* PCB::running = 0;
volatile PCB* PCB::kernelPCB = 0;

volatile PCB::Node* PCB::root = 0;
volatile int PCB::Node::numActive = 0;

