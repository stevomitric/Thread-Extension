/*
 * queue.cpp
 *
 *  Created on: May 20, 2021
 *      Author: OS1
 */

#include "queue.h"
#include "pcb.h"

void Queue::push(PCB* x) {
	lock
	lockF
	if (root == 0)
		root = new Node(x, 0);
	else {
		volatile Node *tmp = root;
		while (tmp->next) tmp = tmp->next;
		tmp->next = new Node(x, 0);
	}
	sz++;
	unlock
	unlockF
}

Queue::Node* Queue::getFirst() {
	return (Node*)root;
}

PCB* Queue::front() {
	lock
	volatile PCB* res = (root == 0) ? 0 : root->data;
	unlock
	return (PCB*)res;
}

PCB* Queue::pop() {
	lock
	lockF
	volatile PCB* res = (root == 0) ? 0 : root->data;
	if (root != 0) {
		Node* old = (Node*)root;
		root = root->next;
		sz--;
		delete old;
	}
	unlock
	unlockF
	return (PCB*)res;
}

PCB* Queue::popFirstLower(unsigned long tm) {
	lock
	lockF
	volatile PCB* res = 0;
	if (root != 0) {
		if (root->data->semTick != 0 && root->data->semTick <= tm)
			res = this->pop();
		else {
			volatile Node* tmp = root;
			while (tmp->next != 0) {
				if (tmp->next->data->semTick != 0 && tmp->next->data->semTick <= tm) {
					res = tmp->next->data;
					Node* old = (Node*)tmp->next;
					tmp->next = tmp->next->next;
					delete old;
					sz--;
					break;
				}
				tmp = tmp->next;
			}
		}
	}
	unlock
	unlockF
	return (PCB*)res;
}

Queue::~Queue() {
	lock
	while (sz) pop();
	unlock
}

int Queue::size() {
	return sz;
}
