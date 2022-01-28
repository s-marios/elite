#include <stdio.h>
#include "macrolist.h"


void * LFIND(void * head, void * elem, comparator comp)
{
	FOREACHPURE(head) {
		if (comp(element, elem) == 0) {
			return element;
		}
	}

	return NULL;
}

void LAPPEND(void ** head_ptr, void * elem)
{
	ELEMENT_PTR * current = (ELEMENT_PTR *) head_ptr;

	while (*current) {
		current = (ELEMENT_PTR *) &(*current)->next;
	}

	*current = elem;
}

void LPREPEND(void ** head_ptr, void * elem)
{
	ELEMENT_PTR newhead = (ELEMENT_PTR) elem;
	newhead->next = (*head_ptr);
	(*head_ptr) = newhead;
}

void * LREPLACE(void ** head_ptr, void * oldelem, void * newelem)
{
	ELEMENT_PTR current = (ELEMENT_PTR) *head_ptr;
	ELEMENT_PTR pold = (ELEMENT_PTR) oldelem;
	ELEMENT_PTR pnew = (ELEMENT_PTR) newelem;

	if (current == oldelem) {
		(*head_ptr) = newelem;
		current = (ELEMENT_PTR) (*head_ptr);
		current->next = pold->next;
		return oldelem;
	} else {
		FOREACHPURE(current) {
			if (current->next == pold) {
				current->next = newelem;
				pnew->next = pold->next;
				return oldelem;
			}
		}
	}

	return NULL;
}

void * LGET(void * head, uint16_t index)
{
	int i = 0;
	FOREACHPURE(head) {
		if (index == i++) {
			return element;
		}
	}

	return NULL;
}
