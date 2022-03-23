/*
 * queue.h
 *
 *  Created on: May 20, 2021
 *      Author: OS1
 */
#ifndef _QUEUE_H_
#define _QUEUE_H_

class PCB;

typedef int ID;

class Queue {
public:
	Queue() : root(0), sz(0) {}

	~Queue();

	struct Node {
		volatile PCB* data;
		volatile Node* next;
		Node() : data(0), next(0) {}
		Node(PCB*d,Node*n=0) : data(d), next(n) {}
	};

	void push(PCB*);
	PCB* front();
	PCB* pop();
	PCB* popFirstLower(unsigned long x);
	int size();

	Node* getFirst();

private:
	volatile Node* root;
	volatile int sz;
};

#endif
