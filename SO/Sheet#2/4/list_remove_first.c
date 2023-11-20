#include <stdio.h>
#include <stdlib.h>
#include "list.h"


void list_remove_first(list* l) {
	/* assumes list l is not empty */
	node* p = l->first;
	l->first = l->first->next;
	l->size--;
	/* free memory allocated for node p */
	free(p);
}