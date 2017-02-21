#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "rngs.h"  // Generatore di Lehmer multistream
#include "rvgs.h" // Distribuzioni utilizzate
#include "structures.h"
#include "time_events.h"

#define LOWER_EQ 5
#define UPPER_EQ 35

#define INTER_ARRIVAL_T 0.02857 // Tempo medio di interarrivo nuove sessioni (1/35)
#define FS_AVG_T 0.00456	// E[D] del fs
#define BE_AVG_T 0.00117	// E[D] del be
#define TH_AVG_T 7	// Tempo medio di attesa nel nodo di think

#define ABDR_THRESHOLD 0.85 // Soglia utilizzazione fs per attivazione ab_dr
#define ABDR_FLOOR 0.75 // Soglia sotto la quale in caso di omm attivato il sistema torna a condizioni standard (ab_dr disattivato)

/**----GESTIONE DROP E ABORT----**/

bool omm = false; // Overload Managment Mechanism (TUNABLE a inizio programma)
bool ab_dr = false; // Abort e Drop attivi se omm è true e se l'utilizzazione supera l'85%

/**----FINE GESTIONE DROP E ABORT----**/

/**----STATO----**/

/*Variabili di stato*/
long session_fifo_fs = 0; // Numero di sessioni in fifo a fs
int xfs = 0; // Stato di occupazione del servente del fs (0 o 1)
long session_fifo_be = 0; // Numero di sessioni in fifo a be
int xbe = 0; // Stato di occpuazione del servente del be (0 o 1)
long session_th = 0; // Numero di sessioni nel nodo di think
/*Fine variabili di stato*/

long total_generated_sessions = 0; // Totale delle sessioni create (anche se droppate)
long total_completed_sessions = 0; // Totale sessioni completate
long total_dropped_sessions = 0; // Totale sessioni droppate

long total_processed_requests = 0; // Totale delle richieste transitate per il be + richieste abortite (nodo di think)
long total_aborted_requests = 0; // Totale richieste abortite

long num_ended_req_fs = 0; // Richieste espletate dal fs
long num_ended_req_be = 0; // Richieste espletate dal be

/**----FINE STATO----**/

/**----METRICHE MISURATE----**/

struct Metrics{
	double sys_resp; // Job average
	double sys_thr; // (total_completed_sessions/cl->current)

	double fs_resp; // Job average
	double fs_thr; // (num_ended_req_fs/cl->current)
	double fs_util; // Area

	double be_resp; // Job average
	double be_thr; // (num_ended_req_be/cl->current)
	double be_util; // Area

	double drop_ratio; // (total_dropped_sessions/total_generated_sessions)
	double abort_ratio; // (total_aborted_requests/total_processed_requests)
};

void reset_metrics(struct Metrics* m)
{
	m->sys_resp = 0.0;
	m->sys_thr = 0.0;
	m->fs_resp = 0.0;
	m->fs_thr = 0.0;
	m->fs_util = 0.0;
	m->be_resp = 0.0;
	m->be_thr = 0.0;
	m->be_util = 0.0;
	m->drop_ratio = 0.0;
	m->abort_ratio = 0.0;
}

struct Metrics* create_metrics(void)
{
	struct Metrics* m = (struct Metrics*)malloc(sizeof(struct Metrics));

	if(m == NULL)
	{
		perror("Error in malloc() of create_metrics!\n");
		exit(1);
	}

	reset_metrics(m);

	return m;
}

void destroy_metrics(struct Metrics** m)
{
	/*Rimuove dallo heap una metrica*/
	if(*m != NULL)
	{
		free(*m);
		*m = NULL;
	}
}
/**----FINE METRICHE MISURATE----**/

/**----GET INTER-TIME MULTISTREAM----**/

int GetSessionLength(void)
{
	/*Genera la lunghezza (numero di richieste) delle nuove sessioni*/

  SelectStream(0);
  return Equilikely(LOWER_EQ, UPPER_EQ);
}

double GetInterArrival(void)
{
	/*Genera il tempo di interarrivo delle nuove sessioni*/

  SelectStream(1);
  return Exponential((double)INTER_ARRIVAL_T);
}

double GetEndFs(void)
{
	/*Genera il tempo di uscita dal fs*/

  SelectStream(2);
  return Exponential((double)FS_AVG_T);
}

double GetEndBe(void)
{
	/*Genera il tempo di uscita dal be*/

  SelectStream(3);
  return Exponential((double)BE_AVG_T);
}

double GetEndTh(void)
{
	/*Genera il tempo di uscita dal th*/

  SelectStream(4);
  return Exponential((double)TH_AVG_T);
}

/**----GET INTER-TIME MULTISTREAM FINE----**/

/**
					SIMULATION:
**/

int main(void)
{

	printf(" TYPE 1 TO ACTIVATE THE OMM MECHANISM, 0 OTHERWISE [ENTER] \n");

	scanf("%d",&omm);

	double previous;

	//Generazione della struttura che raccoglie le metriche
	struct Metrics* metrics = create_metrics();

	double new_session_time = Exponential(INTER_ARRIVAL_T); // Genera il primo tempo di arrivo
	struct calendar* c = create_calendar(new_session_time); // Tutti i tipi di evento

	struct server* fs = create_server(); // Front End
	struct area* a_fs = create_area(); // Area del Front End
	struct server* be = create_server(); // Back End
	struct area* a_be = create_area(); // Area del Back End

	struct list* thinklist = create_list(); // Coda di think (coda con priorità)

	struct clock* cl = create_clock(c->events_times[min(c)]);
	int index = min(c); // Primo evento (arrivo nuova sessione o sampling)

	struct node *n = malloc(sizeof(intptr_t));

	while(c->events_times[NEW_SESSION_INDEX] < STOP_SIMULATION || xfs + xbe + session_th > 0)
	{
		previous = cl->current;
		cl->current = cl->next;

		//AGGIORNAMENTO METRICHE E AREA:

		//Area fs
		update_x(a_fs, (cl->current - previous)*xfs);
		update_q(a_fs, (cl->current - previous)*(session_fifo_fs));
		update_l(a_fs, (cl->current - previous)*(session_fifo_fs + xfs));
		//Area be
		update_x(a_be ,(cl->current - previous)*xbe);
		update_q(a_be, (cl->current - previous)*(session_fifo_be));
		update_l(a_be, (cl->current - previous)*(session_fifo_be + xbe));

		metrics->fs_util = (a_fs->x)/(cl->current); // Utilizzazione fs
		metrics->be_util = (a_be->x)/(cl->current); // Utilizzazione be

		metrics->fs_thr = (num_ended_req_fs)/(cl->current); // Throughput fs
		metrics->be_thr = (num_ended_req_be)/(cl->current); // Throughput fs

		metrics->sys_thr = (total_completed_sessions/(cl->current)); // Throughput sys

		metrics->drop_ratio = ((double)(total_dropped_sessions)/(double)(total_generated_sessions));
		metrics->abort_ratio = ((double)(total_aborted_requests)/(double)(total_processed_requests));

		//FINE AGGIORNAMENTO METRICHE E AREA

		//ATTIVAZIONE DROP E ABORT:

		if(omm == true)
		{
			if (ab_dr == false && metrics->fs_util >= ABDR_THRESHOLD)
				ab_dr = true;

			else if(ab_dr == true && metrics->fs_util <= ABDR_FLOOR)
				ab_dr = false;
		}

		//FINE ATTIVAZIONE DROP E ABORT

		switch (index) {

			case NEW_SESSION_INDEX:

				total_generated_sessions++; // Anche se droppate

				if(ab_dr == false)
				{
					n = create_node();
					n->length = GetSessionLength(); // Numero di richieste
					n->arrival_FS = cl->current;
					n->sessionStart = cl->current;
					add_new_req(fs, n);
					xfs = 1;
					session_fifo_fs = fs->fifo->size;

					if(fs->fifo->size == 0) {
						c->events_times[EXIT_FS_INDEX] = cl->current + GetEndFs();
						n->ended_FS = c->events_times[EXIT_FS_INDEX];
					}
				}

				else
				{
					total_dropped_sessions++;
				}

				c->events_times[NEW_SESSION_INDEX] = cl->current + GetInterArrival();
				if(c->events_times[NEW_SESSION_INDEX] >= STOP_SIMULATION) {
					c->events_times[NEW_SESSION_INDEX] = 100 * STOP_SIMULATION;
				}

				break;
					/**FINE ARRIVO NUOVA SESSIONE (FINE CASE 0)**/

			case EXIT_FS_INDEX: /**USCITA RICHIESTA DA FS**/

				//printf("EXIT FS\n");

				n = update_internal_node(fs);

				if(fs->internal_node != NULL) {
					c->events_times[EXIT_FS_INDEX] = cl->current + GetEndFs();
					fs->internal_node->ended_FS = c->events_times[EXIT_FS_INDEX];
				} else {
					c->events_times[EXIT_FS_INDEX] = 100 * STOP_SIMULATION;
					xfs = 0;
				}

				session_fifo_fs = fs->fifo->size;
				num_ended_req_fs++;
				//AGGIORNAMENTO METRICA fs_resp (TRAMITE WELFORD ONE PASS)
				(metrics->fs_resp) = (metrics->fs_resp) + ((n->ended_FS - n->arrival_FS) - (metrics->fs_resp))/num_ended_req_fs;

				add_new_req(be, n);

				xbe = 1;
				session_fifo_be = be->fifo->size;

				if(be->fifo->size == 0) {
					c->events_times[EXIT_BE_INDEX] = cl->current + GetEndBe();
					n->ended = c->events_times[EXIT_BE_INDEX];
				}

				break;
					/**FINE USCITA RICHIESTA DA FS (FINE CASE 1)**/

			case EXIT_BE_INDEX: /**USCITA RICHIESTA DA BE**/
				//printf("EXIT BE\n");

				n = update_internal_node(be);
				if(be->internal_node != NULL) {
					c->events_times[EXIT_BE_INDEX] = cl->current + GetEndBe();
					be->internal_node->ended = c->events_times[EXIT_BE_INDEX];
				} else {
					c->events_times[EXIT_BE_INDEX] = 100 * STOP_SIMULATION;
					xbe = 0;
				}

				session_fifo_be = be->fifo->size;
				num_ended_req_be++;
				total_processed_requests++;

				n->length--;

				//AGGIORNAMENTO METRICA be_resp (TRAMITE WELFORD ONE PASS)
				(metrics->be_resp) = (metrics->be_resp) + ((n->ended - n->ended_FS) - (metrics->be_resp))/num_ended_req_be;

				if(n->length == 0) {
					n->sessionEnded = cl->current;
					total_completed_sessions++;
					destroy_node(&n);
				} else {
					// DANGER
					n->endThinkTime = cl->current + GetEndTh();
					add_node_THINK_mod(thinklist, n);
					session_th = thinklist->size;
					c->events_times[EXIT_TH_INDEX] = thinklist->head->endThinkTime;
				}

				break;
					/**FINE USCITA RICHIESTA DA BE (FINE CASE 2)**/

			case EXIT_TH_INDEX: /**USCITA RICHIESTA DA TH**/
				//printf("EXIT TH\n");

				n = fetch_node(thinklist);

				if(thinklist->size == 0) {
					c->events_times[EXIT_TH_INDEX] = 100 * STOP_SIMULATION;
				} else {
					c->events_times[EXIT_TH_INDEX] = thinklist->head->endThinkTime;
				}

				if(ab_dr == false)
				{
					n->arrival_FS = cl->current;

					add_new_req(fs, n);
					xfs = 1;
					session_fifo_fs = fs->fifo->size;

					if(fs->fifo->size == 0) {
						c->events_times[EXIT_FS_INDEX] = cl->current + GetEndFs();
						n->ended_FS = c->events_times[EXIT_FS_INDEX];
					}
				}
				// ab_dr attivo -->> ABORT REQUEST
				else
				{
					destroy_node(&n);
					total_dropped_sessions++;
				 	total_aborted_requests++;
					total_processed_requests++;
				}

				session_th = thinklist->size;

				break;
					/**FINE USCITA RICHIESTA DA TH (FINE CASE 3)**/

			case SAMPLING_INDEX:

				printf("utilization fs: %f\n", metrics->fs_util);
				printf("think length: %li\n", session_th);
				printf("fifo fs length: %li\n", session_fifo_fs);
				printf("fifo be length: %li\n", session_fifo_be);
				printf("fs response time: %f\n", metrics->fs_resp);
				//printf("fs_thr: %f\n", metrics->fs_thr);
				//printf("drop: %f\n", metrics->drop_ratio);
				//printf("abort: %f\n", metrics->abort_ratio);
				//printf("be_thr: %f\n", metrics->be_thr);

				set_sample_time(c, cl->current + T_SAMPLE);
				break;

			default:
				printf("Bad event!\n");
		}

		cl->next = c->events_times[min(c)]; // Aggiorno il tempo del next event
		index = min(c); // Il prossimo evento potrebbe essere cambiato

	}

	destroy_calendar(&c);
	destroy_clock(&cl);
	destroy_server(&fs);
	destroy_server(&be);
	destroy_list(&thinklist);
	destroy_area(&a_fs);
	destroy_area(&a_be);
	destroy_metrics(&metrics);

	free(n);

	return EXIT_SUCCESS;
}
