/*
 * kernelsem.h
 *
 *  Created on: May 9, 2021
 *      Author: OS1
 */

#ifndef KSEM_H_
#define KSEM_H_

typedef unsigned int Time;

class Queue;
class Semaphore;
class PCB;

class KernelSem {

	// trenutna vrednost semafora koja je zapravo unsigned
	volatile int val;

	Queue* blocked, *blockedWait;

	Semaphore *s;
public:

	KernelSem(int init, Semaphore* se);
	virtual ~KernelSem();

	int wait(Time time);

	void signal();
	void signalTimer();

	int getVal() const;

	friend class PCB;
};

void semTick();


#endif /* KSEM_H_ */
