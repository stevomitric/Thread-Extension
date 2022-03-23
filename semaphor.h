/*
 * semaphore.h
 *
 *  Created on: May 9, 2021
 *      Author: OS1
 */

#ifndef SEMAPHOR_H_
#define SEMAPHOR_H_

typedef unsigned int Time;

class KernelSem;
class PCB;

class Semaphore {
public:
	Semaphore (int init=1);
	virtual ~Semaphore ();

	virtual int wait (Time maxTimeToWait);
	virtual void signal();

	int val () const; // Returns the current value of the semaphore

private:
	KernelSem* sem;
	friend class PCB;
};

#endif /* SEMAPHOR_H_ */
