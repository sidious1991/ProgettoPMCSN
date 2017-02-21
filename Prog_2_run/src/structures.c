#include <stdio.h>
#include <stdlib.h>
#include "structures.h"

struct list* fifo;

/*----NODI----*/

struct node* create_node()
{
	/*Creazione di Node*/

	struct node* n = (struct node*)malloc(sizeof(struct node));

	if(n==NULL) {
    		perror("Error in malloc of create_node()\n");
    		exit(1);
  	}

	n->next = NULL;
	n->length = 0;
	n->arrival_FS = 0;
	n->ended_FS = 0;
	n->ended = 0;
	n->sessionStart = 0;
	n->sessionEnded = 0;

	n->endThinkTime = 0;

	return n;
}

void destroy_node(struct node** n)
{
	/*Rimuove dallo heap un nodo*/

	if(*n != NULL)
	{
		free(*n);
		*n = NULL;
	}
}

/*----LISTE----*/

struct list* create_list()
{
	/*Creazione di lista vuota*/

	struct list* l = (struct list*)malloc(sizeof(struct list));

	if(l==NULL) {
    		perror("Error in malloc of create_list()\n");
    		exit(1);
  	}

	l->head = NULL;
	l->tail = NULL;
	l->size = 0;

	return l;
}

void add_node_FIFO_mod(struct list* l, struct node* n)
{
	/*Mette in coda il nodo n alla coda FIFO l*/

	if(n == NULL || l == NULL)
	{
		perror("Null pointers in add_node_FIFO_mod() parameters!\n");
		exit(1);
	}

	else if(l->tail == NULL) // FIFO vuota
	{
		l->tail = n;
		l->head = n;
		l->size += 1;
	}

	else // Il nodo si mette in coda
	{
		(l->tail)->next = n;
		l->tail = n;
		l->size += 1;
	}
}

void add_node_THINK_mod(struct list* l, struct node* n)
{
	/*Aggiuge un nodo alla lista nella posizione corretta a seconda del suo tempo di Think*/

	if(n == NULL || l == NULL)
	{
		perror("Null pointers in add_node_THINK_mod() parameters!\n");
		exit(1);
	}

	else if(l->tail == NULL) // lista vuota
	{
		l->tail = n;
		l->head = n;
		l->size += 1;
	}

	else // Il nodo si mette in lista nella posizione che gli compete (lista non vuota)
	{
		struct node* current = l->head; // Parto dalla testa
		struct node* previous = NULL;

		while(current != NULL && current->endThinkTime < n->endThinkTime)
		{
			previous = current;
			current = current->next;
		}

		if(current == NULL) // Inserisco in coda
		{
			(l->tail)->next = n;
			l->tail = n;
			l->size += 1;
		}

		else if(current == l->head) // Inserimento in testa
		{
			n->next = l->head;
			l->head = n;
			l->size += 1;
		}

		else // Inserimento interno alla lista
		{
			previous->next = n;
			n->next = current;
			l->size += 1;
		}
	}
}

struct node* fetch_node(struct list* l)
{
	/*Preleva il nodo in testa alla lista l*/

	struct node* n = NULL;

	if(l == NULL)
	{
		perror("Null pointers in fetch_node() parameters!\n");
		exit(1);
	}

	if(l->head == NULL)
	{
		//printf("%s\n","No Node to fetch!");
	}

	else if(l->head == l->tail) // Un solo nodo in lista
	{
		n = l->head;
		l->head = NULL;
		l->tail = NULL;
		l->size -= 1;
	}

	else // Più di un nodo in lista
	{
		n = l->head;
		l->head = n->next;
		n->next = NULL;
		l->size -= 1;
	}

	return n;
}

void destroy_list(struct list** l)
{
	/*Rimuove dallo heap una lista e tutti i nodi ad essa associati*/

	if(*l != NULL)
	{
		struct node* n = NULL;

		while((*l)->size > 0)
		{
			n = fetch_node(*l);
			destroy_node(&n);
		}

		free(*l);
		*l = NULL;
	}
}

/*----SERVER----*/

struct server* create_server()
{
	/*Creazione di Server*/

	struct server* s = (struct server*)malloc(sizeof(struct server));

	if(s==NULL) {
    		perror("Error in malloc of create_server()\n");
    		exit(1);
  	}

	s->fifo = create_list(); // FIFO vuota inizialmente
	s->internal_node = NULL;

	return s;
}

struct node* update_internal_node(struct server* s)
{
	/*Estrae internal_node e (se la fifo non è vuota) fa fetch sulla fifo, aggiorna internal_node*/

	if(s == NULL)
	{
		perror("Null pointers in update_internal_node() parameters!\n");
		exit(1);
	}

	struct node* old = NULL;

	if(s->internal_node == NULL)
	{
		printf("%s\n","No internal node to update");
	}

	else // Se vi è qualche richiesta in lavorazione
	{
		old = s->internal_node;
		s->internal_node = fetch_node(s->fifo);
	}

	return old;
}

void add_new_req(struct server* s, struct node* n)
{
		/*Inserisce nel server il nodo n in coda o nel servente (se esso è vuoto)*/

		if(s == NULL || n == NULL)
		{
			perror("Null pointers in add_new_req() parameters!\n");
			exit(1);
		}

		if(s->internal_node == NULL) // Il nodo n entra direttamente nel servente (fifo vuota)
			s->internal_node = n;

		else	// Il nodo entra in fifo
			add_node_FIFO_mod(s->fifo, n);

}

void destroy_server(struct server** s)
{
	/*Rimuove dallo heap un server*/

	if(*s != NULL)
	{
		destroy_list(&((*s)->fifo));

		destroy_node(&((*s)->internal_node));

		free(*s);
		*s = NULL;
	}
}

/*----AREA----*/

struct area* create_area()
{
	/*Creazione di Area*/

	struct area* a = (struct area*)malloc(sizeof(struct area));

	if(a==NULL) {
    		perror("Error in malloc of create_area()\n");
    		exit(1);
  	}

	a->x = 0;
	a->q = 0;
	a->l = 0;

	return a;
}

void update_x(struct area* a, double x)
{
	/*Aggiorna x*/

	if(a == NULL)
	{
		perror("Null pointers in update_x() parameters!\n");
		exit(1);
	}

	a->x += x;
}

void update_q(struct area* a, double q)
{
	/*Aggiorna q*/

	if(a == NULL)
	{
		perror("Null pointers in update_q() parameters!\n");
		exit(1);
	}

	a->q += q;
}

void update_l(struct area* a, double l)
{
	/*Aggiorna l*/

	if(a == NULL)
	{
		perror("Null pointers in update_l() parameters!\n");
		exit(1);
	}

	a->l += l;
}

void destroy_area(struct area** a)
{
	/*Rimuove la struttura area dallo heap*/

	if(*a != NULL)
	{
		free(*a);
		*a = NULL;
	}
}
