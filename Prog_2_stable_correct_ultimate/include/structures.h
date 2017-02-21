#ifndef _STRUCTURES_
#define _STRUCTURES_

/*----Modella la sessione----*/
struct node
{
	struct node* next;

	long    length;	//Lunghezza (rimanente) delle richieste della sessione

	double arrival_FS;	//Tempo (assoluto) di arrivo della richiesta al FS
	double ended_FS;	//Tempo (assoluto) di uscita dal FS = Tempo di arrivo al BE
	double ended;	//Tempo (assoluto) di uscita della richiesta dal BE

	double sessionStart;	//Tempo (assoluto) di inizio sessione
	double sessionEnded;	//Tempo (assoluto) di fine sessione

	double endThinkTime;	//Tempo (assoluto) di think del nodo di thinkTime
};

struct node* create_node();
void destroy_node(struct node** n);

/*----Struttura per la modellazione di liste----*/
struct list
{
    struct node* head;
    struct node* tail;
    int size; // Numero di Node in coda
};

struct list* create_list();
void destroy_list(struct list** l);

/*----Modellazione delle list in modalità FIFO----*/
void add_node_FIFO_mod(struct list* l, struct node* n);
struct node* fetch_node(struct list* l);

/*----Modellazione delle list in modalità THINK (la funzione fetch node è la stessa)----*/
void add_node_THINK_mod(struct list* l, struct node* n);

/*----Modella server di fs o di be----*/
struct server
{
	struct node* internal_node; // Richiesta in lavorazione
	struct list* fifo;
};

struct server* create_server();
struct node* update_internal_node(struct server* s);
void add_new_req(struct server* s, struct node* n);
void destroy_server(struct server** s);

/*----Modella quantità cumulative time-dependent utili per il monitoraggio dell'occupazione dei nodi----*/
struct area
{
	double x;
	double q;
	double l;
};

struct area* create_area();
void update_x(struct area* a, double x);
void update_q(struct area* a, double q);
void update_l(struct area* a, double l);
void destroy_area(struct area** a);


#endif
