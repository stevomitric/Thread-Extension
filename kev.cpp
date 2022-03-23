/*
 * kev.cpp
 *
 *  Created on: May 10, 2021
 *      Author: OS1
 */


#include "kev.h"
#include "event.h"
#include "pcb.h"
#include "thread.h"
#include "ivtentry.h"
#include <IOSTREAM.H>

/*
 * Inicijalizujem kernelov dogadjaj
 */
KernelEv::KernelEv(unsigned char x, Event *ev) {
	ent = x;
	this->ev = ev;
	val = 0;

	// setujem prekidne rutine
	IVTEntry::intr[ent]->setInterrupt();
	// linkujem ivtentry sa ovim dogadjajem, kako bi on znao da pozove moj signal
	IVTEntry::intr[ent]->setKernelEv(this);

	created = (PCB*)PCB::running;
}

/*
 * Reci IVEntry objektu da ne poziva mene vise jer sam obrisan
 */
KernelEv::~KernelEv() {
	IVTEntry::intr[ent]->setKernelEv(0);
}

/*
 * Setujem na 1 da ne bi prekoracio
 */
void KernelEv::signal() {
	lock
	val = 1;
	unlock
}

/*
 * Cekam dok signal ne postavi vrednost na 1
 * Dok cekam, menjam kontekst stalno kako ne bi gubio procesorsko vreme bezveze
 */
void KernelEv::wait() {
	if (created != (PCB*)PCB::running) return;
	lock
	while (1) {
		if (val == 1) break;

		unlock
		dispatch(); // ovde radimo scheduler::put i get
		lock
	}
	val = 0;
	unlock
}
