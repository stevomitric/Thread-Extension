/*
 * ivtentry.h
 *
 *  Created on: May 10, 2021
 *      Author: OS1
 */

#ifndef IVTENTRY_H_
#define IVTENTRY_H_

#define PREPAREENTRY(in, out) \
void interrupt intr##in (...); \
volatile IVTEntry* introbj##in = new IVTEntry( (in), (out), intr##in ); \
void interrupt intr##in (...) { \
	introbj##in->signal(); \
	introbj##in->oldInterrupt(); \
	dispatch(); \
}

class KernelEv;

typedef void interrupt (*pint)(...);

class IVTEntry {
	unsigned char in;
	unsigned old;

	// drzim referencu na prehodnu i staru rutinu
	pint oldintr, newintr;

	// referenca na dogadjaj koji pozivam kada se desi prekid
	KernelEv* call;


public:
	IVTEntry(unsigned char in, unsigned old, pint newintr);

	void signal();
	unsigned sigOld();

	void oldInterrupt();
	void setInterrupt();
	void setKernelEv(KernelEv* call);

	static volatile IVTEntry* intr[256];

	friend KernelEv;

	~IVTEntry();
};

#endif /* IVTENTRY_H_ */
