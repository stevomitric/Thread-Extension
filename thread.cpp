#include <IOSTREAM.H>
#include <DOS.H>
#include "thread.h"
#include "pcb.h"
#include "SCHEDULE.H"
#include "queue.h"
#include <STDIO.H>
/*
 * Konstruktor klase Thread koji kreira novi thread, a ne i pokrece ga
 */
Thread::Thread(StackSize stackSize, Time timeSlice) {
	lock
	lockF
	// proveri da li je velicina steka veca od dozvoljene
	if (stackSize > MAX_STACK_SIZE_2B)
		stackSize = MAX_STACK_SIZE_2B;

	myPCB = new PCB(stackSize, timeSlice, this);
	parent = 0;
	_id = _ID++;
	unlock
	unlockF
}

/*
 * pozivajuca nit ceka dok nit nad kojom je pozvana ova funkcija ne zavrsi
 * (Ako nit pozove ovu funkciju sam nad sobom program nikada nece zavrsiti)
 */
void Thread::waitToComplete() {
	lock
	if (myPCB->finished != 1) {
		myPCB->q->push((PCB*)PCB::running);
		PCB::running->dontSchedule = 1;
	}
	unlock
	dispatch();
}

/*
 * Vrati Thread sa datim id-om, ili 0 ako ga nema
 */
Thread* Thread::getThreadById(ID id) {
	lockF
	volatile PCB::Node *tmp = PCB::root;
	while (tmp) {
		if (tmp->data->th && tmp->data->th->getId() == id) {
			Thread*res = tmp->data->th;
			unlockF
			return res;
		}
		tmp = tmp->next;
	}
	unlockF
	return 0;
}

/*
 * Vrati ID niti
 */
ID Thread::getId() {
	return _id;
}

/*
 * Vrati ID od trenutno izvrsavajuce niti
 */
ID Thread::getRunningId() {
	lock
	ID curId = PCB::running->th->getId();
	unlock
	return curId;
}

/*
 * Brisanje niti, sacekaj dok nit zavrsi kako bi je obrisao i updejtu roditelje niti
 */
Thread::~Thread() {
	waitToComplete();

	// updejtuj roditelje
	lock
	lockF
	volatile PCB::Node* tmp = PCB::root;

	while (tmp) {
		if (tmp->data->th && tmp->data->th->parent == this)
			tmp->data->th->parent = parent;
		tmp = tmp->next;
	}

	if (myPCB != 0)
		delete myPCB;
	unlock
	unlockF

}

/*
 * Kreira kontekst PCB-a (inicijalizuje stack)
 * Stavlja nit u red spremnih (ne mora da krene odmah)
 */
void Thread::start() {
	// kreiraj PCB
	lockF
	// TODO: moram handle kada ne uspe da se kreira
	myPCB->createContext();

	PCB::Node::numActive ++;

	// stavi nit u red spremnih
	Scheduler::put(myPCB);
	unlockF
}


/*
 * Dispatch funkcija menja kontekst sa novoizabranom niti
 */
extern volatile int lockFlag;

volatile unsigned tsp, tss;
void dispatch() {
	// u lock-u se cuva PSW
	lock
	if (lockFlag > 0) {
		// nateraj kompajler da pushuje na stek pri pozivu funkcije
		_SI; _DI; _DS;
		// sacuvaj kontekst procesora
		asm {
			push ax
			push bx
			push cx
			push dx
			push es
		}
		// sacuvaj SS i SP
		asm {
			mov tsp, sp
			mov tss, ss
		}
		PCB::running->ss = tss;
		PCB::running->sp = tsp;

		if (PCB::running->finished == 0 && PCB::running->dontSchedule == 0)
			Scheduler::put((PCB*)PCB::running);
		//if (PCB::running->dontSchedule == 1)
		//	PCB::running->dontSchedule = 0;

		PCB::running = Scheduler::get();
		if (PCB::running == 0) PCB::running = PCB::kernelPCB;
		//cout << "Context siwtch thread: " << PCB::running->th << endl;
		PCB::running->timeSlice = PCB::running->origTimeSlice;

		tsp = PCB::running->sp;
		tss = PCB::running->ss;

		asm {
			// take new sp
			mov ss, tss
			mov sp, tsp
		}
		// restauriraj kontekst
		asm {
			pop es
			pop dx
			pop cx
			pop bx
			pop ax
		}
	}
	unlock
}


/*
 * Kreiranje nove niti koji prekopira sadrzaj steka trenutno izvrsavajuce
 */
volatile Thread* tmpSwitch;
volatile int stackPopulated, tmp;
volatile int stack1, stack2;
volatile int ss1, ss2, sdif =0;;
volatile unsigned tsi, tdi, tds, tpsw, obp, dist, nbp;
volatile unsigned long tmp2;
ID Thread::fork() {
	lock

	_SI;_DI;// nateraj kompajler da ih stavi na stek pri pozivu fork
	// Dohvati vrednosti DS,SI,DI i PSW-a sa stacka (psw postavlja lock a ds/si/di implicitno kompajler)
	asm {
		push ax
		mov ax, word ptr [bp-2]
		mov tsi, ax
		mov ax, word ptr [bp-4]
		mov tdi, ax
		mov ax, word ptr [bp-6]
		mov tds, ax
		mov ax, word ptr [bp-8]
		mov tpsw, ax
		pop ax
	}
	// kreiraj novu nit, i prekopiraj stek
	lock
	lockF
	tmpSwitch = PCB::running->th->clone();
	unlock
	unlockF

	tmp=0;
	if (tmpSwitch != 0 && tmpSwitch->myPCB != 0)
		tmp = tmpSwitch->myPCB->createContext();
	if (tmp == 1 || tmpSwitch == 0 || tmpSwitch->myPCB == 0) {
		lockF
		if (tmpSwitch == 0) {}
		else if (tmpSwitch->myPCB == 0) {
			delete tmpSwitch;
		}
		else if (tmp) {
			tmpSwitch->myPCB->finished = 1; // stavljam fleg kako ne bi cekao na waittocomplete
			delete tmpSwitch;
		}
		unlock
		unlockF
		return -1;
	}
	tmpSwitch->myPCB->forked = 1;
	tmpSwitch->parent = PCB::running->th;
	for (tmp2 = 0; tmp2 < PCB::running->stackSize && tmp2 < tmpSwitch->myPCB->stackSize; ++tmp2)
		tmpSwitch->myPCB->stack[tmpSwitch->myPCB->stackSize-1-tmp2] = PCB::running->stack[PCB::running->stackSize-1-tmp2];

	// popravi semafore
	if (FORK_INIT_SEMAPHORE_LOCAL) {
		tmp = PCB::fixSemaphores((PCB*)PCB::running, tmpSwitch->myPCB);
		if (tmp == 0) {
			tmpSwitch->myPCB->finished = 1;
			delete tmpSwitch;
			unlock;
			return -1;
		}
	}

	// kopiraj timeSlice
	if (FORK_COPY_TIMESLICE) {
		tmpSwitch->myPCB->timeSlice = tmpSwitch->myPCB->origTimeSlice = PCB::running->origTimeSlice;
	}

	// pronadji razliku izmedju stekova roditelja i deteta
	sdif = (tmpSwitch->myPCB->stackSize - PCB::running->stackSize)*sizeof(unsigned);

	// pocetni offset za stack ove niti i novokreirane niti
	stack1 = FP_OFF(PCB::running->stack + PCB::running->stackSize - 1);
	stack2 = FP_OFF(tmpSwitch->myPCB->stack + tmpSwitch->myPCB->stackSize - 1);
	// segmentni deo za stack ove niti i novokreirane niti
	ss2 = tmpSwitch->myPCB->ss;
	ss1 = PCB::running->ss;

	// pronadji koliko ima elemenata do bp-a (kako bi znao zauzece stack-a)
	stackPopulated = stack1-_BP+1;

	// sacuvaj trenutni stack pointer (stack segment je sacuvan iznad)
	asm {mov tsp, sp}
	// premesti se na stek niti deteta tako sto ces postaviti sp na: pocetak_stacka_deteta - popunjen_deo
	asm {
		mov ax, stack2
		sub ax, stackPopulated
		add ax, 1

		mov nbp, ax				// ovo je zapravo lokacija BP-a niti deteta

		mov sp, ax
		mov ss, ss2
	}
	// dodaj kontext za dispatch novoj niti
	asm {
		push tsi
		push tdi
		push tds			// ds
		push tpsw			// psw
		mov ax, 0			// stavi povratnu vrednost forka na 0 (dete)
		push ax				//
		push bx
		push cx
		push dx
		push es

		mov tmp, sp			// zapamtim gde je SP niti deteta da stavim u PCB
	}
	// vrati se na stari stack (stack roditelja)
	asm {
		mov sp, tsp
		mov ss, ss1
	}
	// podesi PCB stack pointer za nit deteta
	tmpSwitch->myPCB->sp = tmp;

	// ------------------------- popravi BP --------------------- //
	// sacuvaj BP i ES na stek jer cu da ih koristim
	asm {
		push bp
		push es
		mov ax, ss2		//
		mov es, ax		// neka mi ES registar pokazuje na segmentni deo niti deteta, kako bi mogo zapisivati na njen stek
	}
	loop:		// labela mora biti van asemblija (mora biti C style labela)
	asm {
		mov ax, word ptr [bp]		// dohvati stari BP niti roditelja
		mov obp, ax					// (on se nalazi gde ukazuje trenutni BP)

		mov ax, obp					//
		sub ax, bp					//
		mov dist, ax				// nadji razliku izmedju trenutnog BP-a i starog BP-a

		mov bp, obp					// stari BP mogu postaviti kao trenutni (da bude lakse)

		// sada sacuvaj: obp+sdif na lokaciju ss2:nbp
		mov ax, obp
		add ax, sdif
		mov bx, nbp
		mov word ptr es:bx, ax

		// inkrementiraj nbp za dist
		mov ax, nbp
		add ax, dist
		mov nbp, ax

		// proveri da li je BP=0, ako jeste to je signal kraja (pocetni BP da je jednak 0, sam podesio kada kreiram PCB)
		mov ax, bp
		add ax, 0
		jnz loop:
	}
	asm {
		pop es
		pop bp
	}
	// ------------------------------------------------------------ //

	// markiraj da je nit aktivna i dodaj je u scheduler
	PCB::Node::numActive ++;
	Scheduler::put(tmpSwitch->myPCB);

	// prvo stavim povratnu vrednost u globalnu promenjivu tmp a zatim iz tmp u akumulator
	// ovo radim kako bi izbegao return van 'unlock'-a
	tmp = tmpSwitch->getId();
	asm mov ax, tmp
	unlock
}

void Thread::exit() {
	PCB::quit();
}

/*
 * pozivajuca nit ceka dok joj se svi potomci ne zavrse
 */
void Thread::waitForForkChildren() {
	lock
	int finished = 0;

	while(1) {
		// proveri da li su sve niti zavrsile
		volatile PCB::Node *tmp = PCB::root;
		finished = 1;

		// prodji kroz sve niti
		while (tmp != 0) {
			// ako je pcb od kernelniti
			if (!tmp->data->th) {tmp=tmp->next;continue;}

			//cout << "provera niti: " << tmp->data->th->getId() << endl;
			// nit nije zavrsila
			if (!tmp->data->finished) {

				// proveri je li potomak
				volatile Thread *p = tmp->data->th->parent;
				//cout << "parent: " << p->getId() << " za nit: " << tmp->data->th->getId() << endl;
				while (p) {

					// nit je potomak running niti i nije zavrsila
					if (p == PCB::running->th) {
						finished = 0;
						break;
					}
					p = p->parent;
				}
			}

			tmp = tmp->next;
		}


		if (finished) break;
		unlock
		dispatch();
		lock
	}

	unlock
}

/*
 * Polimorfno kopiranje objekta. Korisnik bi trebao override ovu metodu sa telom: return new MojThread();
 */
Thread* Thread::clone() const {}

// inicijalizuj staticki identifikator
ID Thread::_ID = 0;
