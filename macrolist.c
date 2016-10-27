#include <stdio.h>
#include "macrolist.h"


void * LFIND(void * head, void * elem, comparator comp) {
	FOREACHPURE(head)
	{
		if (comp(element, elem) == 0) {
			return element;
		}
	}
	return NULL;
}

void * LREPLACE(void ** head_ptr, void * oldelem, void * newelem) {
	ELEMENT_PTR phead = (ELEMENT_PTR) (*head_ptr);
	ELEMENT_PTR pold = (ELEMENT_PTR) oldelem;
	ELEMENT_PTR pnew = (ELEMENT_PTR) newelem;
	//TODO error checking
	if (phead == oldelem) {
		(*head_ptr) = newelem;
		phead = (ELEMENT_PTR) (*head_ptr);
		phead->next = pold->next;
		return oldelem;
	} else {
		FOREACHPURE(phead)
		{
			if (phead->next == pold) {
				phead->next = newelem;
				pnew->next = pold->next;
				return oldelem;
			}
		}
	}
	return NULL;
}

void * LGET(void * head, uint index) {
	int i = 0;
	FOREACHPURE(head)
	{
		if (index == i++) {
			return element;
		}
	}
	return NULL;
}
