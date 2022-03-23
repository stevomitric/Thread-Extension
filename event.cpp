/*
 * event.cpp
 *
 *  Created on: May 10, 2021
 *      Author: OS1
 */

#include "event.h"
#include "kev.h"
#include "pcb.h"

/*
 * Kreiraj novu internu strukturu dogadjaja
 */
Event::Event(IVTNo ivtNo) {
	lock
	lockF
	myImpl = new KernelEv(ivtNo, this);
	unlock
	unlockF
}


Event::~Event() {
	lock
	lockF
	delete myImpl;
	unlock
	unlockF
}

/*
 * Samo prosledi zahtev internoj strukturi
 */
void Event::wait() {
	myImpl->wait();
}

/*
 * samo prosledi zahtev internoj strukturi
 */
void Event::signal() {
	myImpl->signal();
}
