/*
 * thread.h
 *
 *  Created on: May 5, 2021
 *      Author: OS1
 */

#ifndef THREAD_H_
#define THREAD_H_
#define MAX_STACK_SIZE_2B 32766

typedef unsigned long StackSize;
const StackSize defaultStackSize = 4096;

typedef unsigned int Time; 				// time, x 55ms
const Time defaultTimeSlice = 2; 		// default = 2*55ms

typedef int ID;

class PCB; // Kernel's implementation of a user's thread

class Thread {
public:
	void start();
	void waitToComplete();

	virtual ~Thread();

	ID getId();

	static ID getRunningId();
	static Thread * getThreadById(ID id);

	//
	// Zadatak 2.
	//
	static ID fork();
	static void exit();
	static void waitForForkChildren();
	virtual Thread* clone() const;
	//

protected:
	friend class PCB;

	Thread (StackSize stackSize = defaultStackSize, Time timeSlice = defaultTimeSlice);

	virtual void run() {}

private:
	PCB* myPCB;
	volatile Thread* parent;

	static ID _ID;
	ID _id;
};

void dispatch ();


#endif /* THREAD_H_ */
