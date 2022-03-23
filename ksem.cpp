/*
 * kernelsem.cpp
 *
 *  Created on: May 9, 2021
 *      Author: OS1
 */

#include <IOSTREAM.H>
#include "ksem.h"
#include "pcb.h"
#include "thread.h"
#include "queue.h"
#include "SCHEDULE.H"
#include "semaphor.h"
/*
 * getTime funkcija koja vraca trenutno 'vreme' u tick-ovima proteklih od pocetka programa
 * (u kernel.cpp definisana)
 */
extern unsigned long getTime();

/*
 * Registruj semafor u kernelu kako bi tick ga pozivao
 */
extern void registerSem(KernelSem*);
extern void unregisterSem(KernelSem*);

/*
 * Inicijalizacija, postavljanje val-a na zadatu vrednost
 */
KernelSem::KernelSem(int init, Semaphore *se) {
	lock
	lockF
	s = se;
	val = init;
	blocked = new Queue();
	blockedWait = new Queue();
	registerSem(this);
	unlock
	unlockF
}

/*
 * Ako je neka nit zablokirana u vreme brisanja semafora moze se svasta destiti
 * Da li sam trebao sacekati da se sve niti odblokiraju ?
 */
KernelSem::~KernelSem() {
	lock
	lockF
	unregisterSem(this);
	// obezbedi da se blokirane niti na semaforu odblokiraju
	while (val==0)
		signal();
	if (blocked != 0)
		delete blocked;
	if (blockedWait != 0)
		delete blockedWait;
	unlock
	unlockF
}

/*
 * Signalizacija samo povecava val za jedan i deblokira jednu nit (vrati u scheduler)
 */
void KernelSem::signal() {
	lock
	// proveri da li ima u blocked:
	if (blocked->size() > 0) {
		PCB* nova = blocked->pop();
		Scheduler::put(nova);
		nova->dontSchedule = 0;
	}
	// proveri da li ima u blockedWait-u a
	else if (blockedWait->size() > 0) {
		PCB* nova = blockedWait->pop();
		Scheduler::put(nova);
		nova->dontSchedule = 0;
	}
	else val++;

	unlock
}

void KernelSem::signalTimer() {
	lock
	while (1) {
		PCB* nova = blockedWait->popFirstLower(getTime());
		if (nova == 0) break;
		Scheduler::put(nova);
		nova->dontSchedule = 0; nova->semSig = 0;
	}
	unlock
}

/*
 * Wait funkcionise tako sto:
 * -> ako je val>0, smanji za jedan i izadji odma
 * -> ako je val=0, sacekaj dok val ne bude 1 (ili >0), pa smanji za jedan i izadji odma
 *
 * ako vidim da nit treba da ceka na wait-u, odma pozovi dispatch
 * unlock i lock kod dispatch-a sluze kako ne bi blokirali timer
 */
int KernelSem::wait(Time time) {
	lock

	PCB::running->semSig = 1;
	if (val == 0) {
		// podesi vreme izlaza u semTick
		PCB::running->semTick = (time == 0 ? 0 : getTime() + time);
		// dodaj je u blocker red i reci da se ne stavlja u scheduler
		if (time == 0)
			blocked->push((PCB*)PCB::running);
		else
			blockedWait->push((PCB*)PCB::running);
		PCB::running->dontSchedule = 1;
		// kada se vratimo iz dispatch-a, ova nit ce biti unblocked
		dispatch();
	}
	else {
		val -= 1;
	}

	unlock

	return PCB::running->semSig;
}

/*
 * Kako bi ocuvao 'tradicionalni' val sa predavanja, koji zalazi u minus, moram pratiti i numBlocked
 */
int KernelSem::getVal() const {
	return (val == 0) ? -(blocked->size()+blockedWait->size()) : val;
}
