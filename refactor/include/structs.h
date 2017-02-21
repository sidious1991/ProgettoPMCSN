#ifndef _STRUCTS_
#define _STRUCTS_

#include "const.h"

struct node {
	struct node* next;
	long length;
	double arrival_FE;
	double arrival_BE;
	double ended_BE;
	double sessionStart;
	double sessionEnded;
	double thinkTime;
	int class;
};

struct node* create_node();
void destroy_node(struct node** n);

struct list {
    struct node* head;
    struct node* tail;
    int size;
};

struct list* create_list();
void destroy_list(struct list** l);

void add_node(struct list* l, struct node* n);

struct node* fetch_node(struct list* l);

void add_think_node(struct list* l, struct node* n);

struct server {
	struct node* work;
	struct list* queue;
};

struct frontend {
  struct node* work;
  struct list* queue1;
  struct list* queue2;
};

struct server* create_server();
struct frontend* create_frontend();

struct node* update_worker_node(struct server* s);
struct node* update_worker_node_frontend(struct frontend* f);

void add_new_req(struct server* s, struct node* n);
void add_new_req_frontend(struct frontend* f, struct node* n);

void destroy_server(struct server** s);
void destroy_frontend(struct frontend** f);

struct area {
	double service;
	double queue;
	double node;
};

struct area* create_area();
void update_service(struct area* a, double service);
void update_queue(struct area* a, double queue);
void update_node(struct area* a, double node);
void destroy_area(struct area** a);

#endif
