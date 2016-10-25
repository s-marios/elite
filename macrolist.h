#ifndef MACROLIST
#define MACROLIST

#include <stdint.h>

typedef struct {
	void * next;
} ELEMENT, *ELEMENT_PTR;

#define LHASNEXT(elem) (elem)->next

#define LAPPEND(head, elem) do { 				\
	if (LHASNEXT(head) == NULL) { 				\
		head->next = elem;						\
	} else { 									\
		ELEMENT_PTR tmp = (ELEMENT_PTR) head;	\
		while (LHASNEXT(tmp)) {					\
			tmp = tmp->next;					\
		}										\
		tmp->next = (elem);						\
	}											\
} while (0)

#define LPREPEND(head, elem) do { 				\
	elem->next = head;							\
	head = elem;								\
} while (0)

#define LNEXT(elem) LHASNEXT(elem); 		\
	do { 									\
		elem = elem->next; 					\
	} while (0)

typedef int (*comparator)(void *, void *);

#define FOREACHPURE(head) for (ELEMENT_PTR element = (ELEMENT_PTR) head; element; element = element->next)
#define FOREACH(head, type) for (type element = head; element; element = (type) ((ELEMENT_PTR) element)->next)
#define FOREACHWITH(head, type, name) for (type name = head; name; name = (type) (ELEMENT_PTR) name->next)

void * LFIND(void * head, void * elem, comparator comp);
void * LREPLACE(void ** head_ptr, void * oldelem, void * newelem);
void * LGET(void * head, uint index);
//file end
#endif
