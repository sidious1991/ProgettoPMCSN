#include <stdio.h>
#include <stdlib.h>
#include "structs.h"

struct list* queue;

struct node* create_node() {
	struct node* n = (struct node*) malloc(sizeof(struct node));

	if(n==NULL) {
    perror("Error in malloc of create_node()\n");
    exit(1);
  }

	n->next = NULL;
	n->length = 0;
	n->arrival_FE = 0;
	n->arrival_BE = 0;
	n->ended_BE = 0;
	n->sessionStart = 0;
	n->sessionEnded = 0;

	n->thinkTime = 0;

	n->class = -1;

	return n;
}

void destroy_node(struct node** n) {
	if(*n != NULL)
	{
		free(*n);
		*n = NULL;
	}
}

struct list* create_list() {
	struct list* l = (struct list*) malloc(sizeof(struct list));

	if(l==NULL) {
    		perror("Error in malloc of create_list()\n");
    		exit(1);
  	}

	l->head = NULL;
	l->tail = NULL;
	l->size = 0;

	return l;
}

void add_node(struct list* l, struct node* n) {
	if(n == NULL || l == NULL)
	{
		perror("Null pointers in add_node() parameters!\n");
		exit(1);
	}

	else if(l->tail == NULL)
	{
		l->tail = n;
		l->head = n;
		l->size += 1;
	}

	else
	{
		(l->tail)->next = n;
		l->tail = n;
		l->size += 1;
	}
}

void add_think_node(struct list* l, struct node* n) {
	if(n == NULL || l == NULL)
	{
		perror("Null pointers in add_node()\n");
		exit(1);
	}

	else if(l->tail == NULL)
	{
		l->tail = n;
		l->head = n;
		l->size += 1;
	}

	else
	{
		struct node* current = l->head;
		struct node* previous = NULL;

		while(current != NULL && current->thinkTime < n->thinkTime)
		{
			previous = current;
			current = current->next;
		}

		if(current == NULL)
		{
			(l->tail)->next = n;
			l->tail = n;
			l->size += 1;
		}

		else if(current == l->head)
		{
			n->next = l->head;
			l->head = n;
			l->size += 1;
		}

		else
		{
			previous->next = n;
			n->next = current;
			l->size += 1;
		}
	}
}

struct node* fetch_node(struct list* l) {
	struct node* n = NULL;

	if(l == NULL)
	{
		perror("Null pointers in fetch_node()\n");
		exit(1);
	}

	else if(l->head == l->tail)
	{
		n = l->head;
		l->head = NULL;
		l->tail = NULL;
		l->size -= 1;
	}

	else
	{
		n = l->head;
		l->head = n->next;
		n->next = NULL;
		l->size -= 1;
	}

	return n;
}

void destroy_list(struct list** l) {

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

struct server* create_server() {
	struct server* s = (struct server*) malloc(sizeof(struct server));

	if(s == NULL) {
    perror("Error in malloc of create_server()\n");
    exit(1);
  }

	s->queue = create_list();
	s->work = NULL;

	return s;
}

struct frontend* create_frontend() {
	struct frontend* f = (struct frontend*) malloc(sizeof(struct frontend));
	if(f == NULL) {
		perror("Error in malloc of create_frontend()");
		exit(1);
	}

	f->queue1 = create_list();
	f->queue2 = create_list();
	f->work = NULL;

	return f;
}

struct node* update_worker_node(struct server* s) {
	if(s == NULL)
	{
		perror("Null pointer in update_internal_node() parameters!\n");
		exit(1);
	}

	struct node* old = NULL;

	if(s->work == NULL)
	{
		printf("%s\n","No internal node to update");
	}

	else
	{
		old = s->work;
		s->work = fetch_node(s->queue);
	}

	return old;
}

struct node* update_worker_node_frontend(struct frontend* f) {
	if(f == NULL)
	{
		perror("Null pointer in update_internal_node()\n");
		exit(1);
	}

	struct node* old = NULL;

	if(f->work == NULL)
	{
		printf("%s\n","No internal node to update");
	} else {

		old = f->work;

		if(f->queue1->size != 0)
		{
			f->work = fetch_node(f->queue1);
		} else {
			f->work = fetch_node(f->queue2);
		}
		
	}

	return old;
}

void add_new_req(struct server* s, struct node* n) {
		
		if(s == NULL || n == NULL)
		{
			perror("Null pointers in add_new_req()\n");
			exit(1);
		}

		if(s->work == NULL)
			s->work = n;

		else
			add_node(s->queue, n);

}

void add_new_req_frontend(struct frontend* f, struct node* n) {
	if(f == NULL || n == NULL) {
		perror("Null pointers in add_new_req_frontend()\n");
		exit(1);
	}

	if(f->work == NULL)
	{
			f->work = n;
	} else if(n->class == CLASS_1) {
		add_node(f->queue1, n);
	} else {
		add_node(f->queue2, n);
	}

}

void destroy_frontend(struct frontend** f) {
	if(*f != NULL)
	{
		destroy_list(&((*f)->queue1));
		destroy_list(&((*f)->queue2));
		destroy_node(&((*f)->work));

		free(*f);
		*f = NULL;
	}
}

void destroy_server(struct server** s) {
	if(*s != NULL)
	{
		destroy_list(&((*s)->queue));

		destroy_node(&((*s)->work));

		free(*s);
		*s = NULL;
	}
}

struct area* create_area() {
	struct area* a = (struct area*)malloc(sizeof(struct area));

	if(a==NULL) {
    		perror("Error in malloc of create_area()\n");
    		exit(1);
  	}

	a->service = 0;
	a->queue = 0;
	a->node = 0;

	return a;
}

void update_service(struct area* a, double service) {
	if(a == NULL)
	{
		perror("Null pointers in update_x()\n");
		exit(1);
	}
	a->service += service;
}

void update_queue(struct area* a, double queue) {
	if(a == NULL)
	{
		perror("Null pointers in update_q()\n");
		exit(1);
	}

	a->queue += queue;
}

void update_node(struct area* a, double node) {
	if(a == NULL)
	{
		perror("Null pointers in update_l()\n");
		exit(1);
	}

	a->node += node;
}

void destroy_area(struct area** a) {
	if(*a != NULL)
	{
		free(*a);
		*a = NULL;
	}
}
