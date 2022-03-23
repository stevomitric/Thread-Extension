/*
 * ivtentry.cpp
 *
 *  Created on: May 10, 2021
 *      Author: OS1
 */

#include "ivtentry.h"
#include "kev.h"
#include "pcb.h"
#include <DOS.H>
#include <IOSTREAM.H>
volatile IVTEntry* IVTEntry::intr[256] = {0};

/*
 * Inicijalizujem ulaz prekida (ne diram prekidne rutine)
 */
IVTEntry::IVTEntry(unsigned char in, unsigned old, pint newintr) {
	intr[this->in = in] = this;
	this->old = old;
	this->oldintr = 0;
	this->newintr = newintr;
	this->call = 0;
}

/*
 * Zapisujem prekidnu rutinu i cuvam staru
 */
void IVTEntry::setInterrupt() {
	// preusmeri ivt
	lock
	lockF
	if (this->oldintr == 0) {
		this->oldintr = getvect(this->in);
		setvect(this->in, this->newintr);
	}
	unlock
	unlockF
}

/*
 * Setujem dogadjaj koji cu da pozivam iz signal-a
 */
void IVTEntry::setKernelEv(KernelEv* call) {
	this->call = call;
	//lock
	//cout << "reset to old interrupt: " << (int)this->in << "-" <<this->oldintr << endl;
	//if (call == 0) setvect(this->in, this->oldintr);
	//unlock
}

/*
 * Pozivam signal dogadjaja
 * Ovu funkciju poziva prekidna rutina (makro definisan u ivtentry.h)
 */
void IVTEntry::signal() {
	if (this->call != 0)
		call->signal();
}

/*
 * Poziva old interrupt ako je podesen flag da to i uradim
 */
void IVTEntry::oldInterrupt() {
	if (this->oldintr && this->old) {
		this->oldintr();
	}
}

/*
 * Kada se brise ivtentry, vrati staru rutinu
 */
IVTEntry::~IVTEntry() {
	lock
	if (this->oldintr)
		setvect(this->in, this->oldintr);
	unlock
}


