/*
 * kev.h
 *
 *  Created on: May 10, 2021
 *      Author: OS1
 */

#ifndef KEV_H_
#define KEV_H_

class Event;
class PCB;

class KernelEv {
	// broj ulaza
	unsigned char ent;

	// Drzim referencu na dogadjaj
	// nije potrebno trenutno dok pisem ovaj komentar, ali mozda zatreba kasnije
	Event *ev;
	// referenca na nit koja je kreirala dogadjaj
	PCB* created;

	// vrednost na binarnom semaforu
	volatile unsigned val;

public:
	KernelEv(unsigned char, Event* ev);
	~KernelEv();

	void wait();

	void signal();
};


#endif /* KEV_H_ */
