/*
 * kernel.cpp
 *
 *  Created on: May 3, 2021
 *      Author: OS1
 */

#include <iostream.h>

#include <DOS.H>"

// Ukljuci potrebne biblioteke
#include "thread.h"
#include "pcb.h"
#include "SCHEDULE.H"
#include "ksem.h"

// definisemo kod neregularnog izvrsavanja main funkcije
#define MAIN_IRREGULAR_FINISH 0xffff

typedef void interrupt (*pint)(...);


/*
 * Omoguci semaforima da se registruju za tick
 */

NodeSem*rootSem = 0;
void registerSem(KernelSem* sem) {
	lock
	lockF
	if (rootSem == 0) rootSem = new NodeSem(sem, 0);
	else rootSem = new NodeSem(sem, rootSem);
	unlock
	unlockF
}
void unregisterSem(KernelSem *sem) {
	lock
	lockF
	if (rootSem != 0) {
		if (rootSem->data == sem) {
			NodeSem* old = rootSem;
			rootSem = rootSem->next;
			delete old;
		}
		else {
			NodeSem* tmp = rootSem;
			while (tmp->next != 0) {
				if (tmp->next->data == sem) {
					NodeSem* old = tmp->next;
					tmp->next = tmp->next->next;
					delete old;
					break;
				}
				tmp = tmp->next;
			}
		}
	}
	unlock
	unlockF
}
void tickSem() {
	lock
	for (NodeSem*i = rootSem; i!=0; i = i->next) {
		i->data->signalTimer();
	}
	unlock

}

/*
 * Obezbedi zabranu promene konteksta bez zabrane prekida
 */
volatile int lockFlag = 1;

/*
 * Funkcija usermain nije vidljiva kernelu, nego on zna (nada se) da ce korisnik da je implementira
 */
extern int userMain(int argc, char *argv[]);

/*
 * Funkcija koja pravi pocetni kontekst main funkcije
 */
void createKernelPCB() {
	// ovo je trenutno izvrsavajuci thread
	lock
	lockF
	PCB::running = new PCB(1024, 0, 0);
	unlockF
	unlock

	PCB::running->createContext();

	PCB::kernelPCB = PCB::running;
	PCB::kernelPCB->dontSchedule=1;
}

/*
 * Kernel obrada za vremenski brojac (pozivm pre korisnickog tick-a)
 */
void tickKer() {
	// unisti sve Thread niti koje naguju
	if (AUTO_DELETE_FORK_CHILDREN) {
		lockF
		lock
		for (PCB::Node *cur = (PCB::Node*)PCB::root; cur != 0; cur = cur->next) {
			if (cur->data->forked == 1 && cur->data->finished == 1) {
				if (cur->data->th != 0) {
					delete cur->data->th;
					break; // lista se deformirala zbog delete
				}
			}
		}
		unlock
		unlockF
	}
}

/*
 * Funkcija koja pruza vreme u tickovima (55ms)
 * Garantovano da ce svaki naredni poziv dati inkrementirajuci broj
 */
volatile unsigned tickCnt = 0;
unsigned long getTime() {
	return tickCnt;
}


/*
 * Funkcija test okruzenja
 */
extern void tick();
//void tick() {}

/*
 * Interrupt rutina za timer koji se poziva svakih 55ms
 * Potrebno je da pozovem prethodnu prekidnu rutinu koju sam premestio na 60h ulaz
 */
void interrupt timer(...) {
	asm int 60h;
	tickCnt ++;
	tickSem();
	tickKer();
	tick();

	if (PCB::running->timeSlice>1)
		PCB::running->timeSlice--;
	else if (PCB::running->timeSlice == 1)
		dispatch();

}


/*
 * Wrapper funkcija koja se poziva iz prekidne rutine sa prosledjenim pritisnutim keycodom
 * (Potrebno je nekako da mapiramo keykode u karaktere ?)
 * (Razlikuje se keykode kada pritisnem 'a' i kada pustim 'a' ?)
 */
void keyPressed(unsigned short x) {
	cout << "key: " << x << endl;
	dispatch();
}

/*
 * Prekidna rutina koja hvata prekide od tastature
 */
void interrupt keyboardISR(...) {
	volatile unsigned short x = 0;
	// dohvati karakter
	asm {
		in al, 60h
		mov byte ptr x, al
	}

	// adaptacija za stare tastature i signalizacija kontroleru periferije da moze opet poslati prekid
	asm {
		in al, 61h
		or al, 10000000b
		out 61h, al
		and al, 01111111b
		out 61h, al

		mov al, 20h
		out 20h, al
	}

	keyPressed(x);
}


/*
 * Preuzimanje prekidnih rutina tajmera i tastature na moje funkcije
 */
pint oldISR, oldTMR;
void init() {
	// za tastaturu
	//oldISR = getvect(9);
	//setvect(9, keyboardISR);

	// za timer
	oldTMR = getvect(8);
	setvect(8, timer);
	setvect(0x60, oldTMR);
}

/*
 * Vracanje starih prekida
 * (Da li je ovo potrebno uopste ?)
 */
void restore() {
	//setvect(9,oldISR);
	setvect(8,oldTMR);
}


class MainThread : public Thread {
	int argc, res;
	char**argv;
public:
	MainThread(int ac, char*av[]) : argc(ac), argv(av) {}
	void run() {
		// u slucaju da main se ne izvrsi regularno (npr pozvan exit)
		res = MAIN_IRREGULAR_FINISH;

		res = userMain(argc, argv);
	}
	~MainThread() {waitToComplete();}
	int getRes() {return res;};
	Thread* clone() const {return new MainThread(argc, argv);}
};

/*
 * Main funkcija kernela koja pokrece korisnicku funkciju
 */
int main(int argc, char *argv[]) {
	lock
	init();
	createKernelPCB();
	unlock

	//int res = userMain(argc, argv);
	MainThread mt(argc, argv);
	mt.start();

	// sacekaj da sve niti zavrse
	while (PCB::Node::allFinished() != 1)
		dispatch();

	lock
	lockF
	//cout << "Zavrsio kernel main sa vrednosti: " << mt.getRes() << " (all threads dead)" << endl;
	restore();
	unlockF
	unlock

	return mt.getRes();
}

