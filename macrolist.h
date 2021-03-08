/**
 * \file
 * Header file for a list implementation.
 */
#ifndef MACROLIST
#define MACROLIST

#include <stdint.h>

/**
 * a basic structure used to represent a list element. Each element has a next
 * pointer to the next element of the list. To create structs that can
 * be used as a list, make sure that the first field of your struct is
 * void * next, so all future casts can succeed.
 */
typedef struct {
	void * next;
} ELEMENT, *ELEMENT_PTR;

/**
 * used to check if another element follows the current one
 */
#define LHASNEXT(elem) (elem)->next

/**
 * Forward towards the next list element.
 */
#define LNEXT(elem) LHASNEXT(elem); 		\
	do { 									\
		elem = elem->next; 					\
	} while (0)

/**
 * typedef for a comparison operator for two elements on a list
 */
typedef int (*comparator)(void *, void *);

/**
 * 'For Each' loop construct. Used to access elements only as #ELEMENT_PTR s
 * thus not very useful apart from counting the number of objects in a list.
 */
#define FOREACHPURE(head) for (ELEMENT_PTR element = (ELEMENT_PTR) head; element; element = element->next)
/**
 * Use this to loop the list, casting each element to a type of "type". For example
 * FOREACH(oHead, OBJ_PTR) {
 * 		//do things here...
 * 		element->eoj = "ABC";
 * }
 * is a loop that casts all the elements to type OBJ_PTR. The current element in
 * the loop is accessible through the variable 'element'.
 */
#define FOREACH(head, type) for (type element = head; element; element = (type) ((ELEMENT_PTR) element)->next)
/**
 * same as #FOREACH but make the accessible element available under the variable 'name'
 */
#define FOREACHWITH(head, type, name) for (type name = head; name; name = (type) (ELEMENT_PTR) name->next)

/**
 * Find an element in a list. The search criteria is usually another struct
 * of the same type with one or more of its fields used as search criteria.
 * The comparator function knows how to use the criteria and is responsible
 * for correct castings.
 *
 * @param head the head of that list
 * @param a pointer to the search criteria
 * @param comp the comparator function
 */
void * LFIND(void * head, void * elem, comparator comp);

/**
 * Replace a property with a new one. Make sure you supply a reference to a
 * struct castable to #ELEMENT_PTR , since we may need to replace the first element
 *
 * @param a pointer to the pointer of the head of a list
 * @param oldelem the element to be replaced
 * @param newelem the new element to replace with
 */
void * LREPLACE(void ** head_ptr, void * oldelem, void * newelem);

/**
 * Index-based element get operation for a list.
 */
void * LGET(void * head, uint16_t index);

/**
 * Append an element to a List. Make sure you supply a reference to a
 * struct castable to #ELEMENT_PTR , since we may need to replace the
 * first element. This can be used to initialize NULL lists for example,
 *  by supplying a pointer to a NULL pointer.
 */
void LAPPEND(void ** head_ptr, void * elem);

//file end
#endif
