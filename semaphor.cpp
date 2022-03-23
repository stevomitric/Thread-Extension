/*
 * semaphore.cpp
 *
 *  Created on: May 9, 2021
 *      Author: OS1
 */

#include "ksem.h"
#include "pcb.h"
#include "semaphor.h"

/*
 * Inicijalizacija semafora, samo kreiram novi kernelov semafor
 */
Semaphore::Semaphore(int init) {
	// kreiraj novi kernelsem
	lock
	lockF
	sem = new KernelSem(init, this);
	unlock
	unlockF
}

/*
 * Samo izbrisi semafor
 */
Semaphore::~Semaphore() {
	lock
	lockF
	if (sem != 0)
		delete sem;
	unlock
	unlockF
}

/*
 * Signalizacija internoj semafor strukturi
 */
void Semaphore::signal() {
	sem->signal();
}

/*
 * pozivanje wait-a internoj semafor strukturi
 */
int Semaphore::wait(Time time) {
	return sem->wait(time);
}

/*
 * Pozivanje val-a internoj semafor strukturi
 */
int Semaphore::val() const {
	return sem->getVal();
}

