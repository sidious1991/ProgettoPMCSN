#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "rngs.h"  // Generatore di Lehmer multistream
#include "rvgs.h" // Distribuzioni utilizzate
#include "rvms.h" // Quantità pivotali
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

/** METRICA SELEZIONATA (TREND) **/

double* trend;
char choice[15]; // Metrica selezionata

/** FINE METRICA SELEZIONATA (TREND) **/

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
long num_entered_be_serv = 0; // Numero richieste entrate nel servente del be in un certo istante di simulazione

/**----RESET STATE----**/

void reset_state(void)
{
	/*Resetta le varabili di stato e quelle di appoggio*/

	ab_dr = false;
	session_fifo_fs = 0;
	xfs = 0;
	session_fifo_be = 0;
	xbe = 0;
	session_th = 0;
	total_generated_sessions = 0;
	total_completed_sessions = 0;
	total_dropped_sessions = 0;
	total_processed_requests = 0;
	total_aborted_requests = 0;
	num_ended_req_fs = 0;
	num_ended_req_be = 0;
	num_entered_be_serv = 0;
}

/**---- FINE RESET STATE----**/

/**----FINE STATO----**/

/**----METRICHE MISURATE----**/

struct Metrics{
	double sys_resp; // Job average
	double sys_thr; // (total_completed_sessions/cl->current)

	double fs_resp; // Job average
	double fs_thr; // (num_ended_req_fs/cl->current)
	double fs_util; // Area

	double be_resp; // Job average
	double be_delay; // Tempo medio in coda del be (Job average)
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
	m->be_delay = 0.0;
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

void simulation(struct Metrics* metrics)
{

	PlantSeeds(-1);

	double previous;


	double new_session_time = GetInterArrival(); // Genera il primo tempo di arrivo
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

					num_entered_be_serv++;
					//AGGIORNAMENTO METRICA be_delay (TRAMITE WELFORD ONE PASS)
					(metrics->be_delay) = (metrics->be_delay) + ((cl->current - n->ended_FS) - (metrics->be_delay))/num_entered_be_serv;
				}

				break;
					/**FINE USCITA RICHIESTA DA FS (FINE CASE 1)**/

			case EXIT_BE_INDEX: /**USCITA RICHIESTA DA BE**/
				//printf("EXIT BE\n");

				n = update_internal_node(be);
				if(be->internal_node != NULL) {
					c->events_times[EXIT_BE_INDEX] = cl->current + GetEndBe();
					be->internal_node->ended = c->events_times[EXIT_BE_INDEX];

					num_entered_be_serv++;
					//AGGIORNAMENTO METRICA be_delay (TRAMITE WELFORD ONE PASS)
					(metrics->be_delay) = (metrics->be_delay) + ((cl->current - be->internal_node->ended_FS) - (metrics->be_delay))/num_entered_be_serv;
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

				//AGGIORNAMENTO METRICA sys_resp (TRAMITE WELFORD ONE PASS)
				(metrics->sys_resp) = (metrics->sys_resp) + ((n->ended - n->arrival_FS) - (metrics->sys_resp))/num_ended_req_be;

				if(n->length == 0) {
					n->sessionEnded = cl->current;
					total_completed_sessions++;
					destroy_node(&n);
				} else {

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

				/** AGGIORNAMENTO DEL TREND TEMPORALE DELLA METRICA SELEZIONATA **/
				if(cl->current <= STOP_SIMULATION)
				{
						if(strcmp(choice,"sys_resp") == 0)
							trend[(int)(cl->current - 1)] += metrics->sys_resp;

						else if (strcmp(choice,"sys_thr") == 0)
							trend[(int)(cl->current - 1)] += metrics->sys_thr;

						else if (strcmp(choice,"fs_resp") == 0)
							trend[(int)(cl->current - 1)] += metrics->fs_resp;

						else if (strcmp(choice,"fs_thr") == 0)
							trend[(int)(cl->current - 1)] += metrics->fs_thr;

						else if (strcmp(choice,"be_resp") == 0)
							trend[(int)(cl->current - 1)] += metrics->be_resp;

						else if (strcmp(choice,"be_delay") == 0)
							trend[(int)(cl->current - 1)] += metrics->be_delay;

						else if (strcmp(choice,"be_thr") == 0)
							trend[(int)(cl->current - 1)] += metrics->be_thr;

						else if (strcmp(choice,"drop_ratio") == 0)
							trend[(int)(cl->current - 1)] += metrics->drop_ratio;

						else // Abort ratio
							trend[(int)(cl->current - 1)] += metrics->abort_ratio;

						//printf("%f\n", trend[(int)(cl->current - 1)]);
				}

				set_sample_time(c, cl->current + T_SAMPLE);
				break;

			default:
				printf("Bad event!\n");
		}

		cl->next = c->events_times[min(c)]; // Aggiorno il tempo del next event
		index = min(c); // Il prossimo evento potrebbe essere cambiato

	}

	//FREE MEMORY

	destroy_calendar(&c);
	destroy_clock(&cl);
	destroy_server(&fs);
	destroy_server(&be);
	destroy_list(&thinklist);
	destroy_area(&a_fs);
	destroy_area(&a_be);

	free(n);
}

/** CONFIDENCE INTERVAL **/

double confidence_interval(int n, double s)
{
	double t = idfStudent(n-1, 0.975);
	double sqnum = sqrt((double) n);
	double sqs = sqrt(s);

	return (t * sqs) / sqnum;
}

/** FINE CONFIDENCE INTERVAL **/

/**
 	MAIN:
**/

int main(void)
{
	printf("TYPE 1 TO ACTIVATE THE OMM MECHANISM, 0 OTHERWISE [ENTER]:\n");

	scanf("%d",&omm);

	printf("\nENTER THE METRIC TO MEASURE (sys_resp / sys_thr / fs_resp / fs_thr / be_resp / be_delay / be_thr / drop_ratio / abort_ratio) [ENTER]:\n");

	scanf("%s", choice); // Selezione della metrica da monitorare (trend temporale)

	printf("\n");

	//Generazione della struttura che raccoglie le metriche (per ogni simulazione)
	struct Metrics* metrics = create_metrics();

	struct Metrics* mean = create_metrics(); // Utile per il calcolo dell'intervallo di confidenza
	struct Metrics* var = create_metrics(); // Utile per il calcolo dell'intervallo di confidenza

	//INIZIALIZZAZIONE TREND METRICA SELEZIONATA (MEDIATA PER OGNI RUN):

	trend = (double*)malloc(10000*sizeof(double));

	if(trend == NULL)
	{
		perror("ERROR IN MALLOC!\n");
	}

	memset(trend, 0.0, sizeof(double)*10000);

	int i;

	for(i = 1 ; i < 51 ; i++)
	{
		printf("SIMULATION IN PROGRESS (RUN %d) \n", i);

		simulation(metrics);

		//VARIANZE:
		var->sys_resp = var->sys_resp + pow((metrics->sys_resp - mean->sys_resp) , 2.0)*(i-1)/i;
		var->sys_thr = var->sys_thr + pow((metrics->sys_thr - mean->sys_thr) , 2.0)*(i-1)/i;

		var->fs_resp = var->fs_resp + pow((metrics->fs_resp - mean->fs_resp) , 2.0)*(i-1)/i;
		var->fs_thr = var->fs_thr + pow((metrics->fs_thr - mean->fs_thr) , 2.0)*(i-1)/i;

		var->be_resp = var->be_resp + pow((metrics->be_resp - mean->be_resp) , 2.0)*(i-1)/i;
		var->be_delay = var->be_delay + pow((metrics->be_delay - mean->be_delay) , 2.0)*(i-1)/i;
		var->be_thr = var->be_thr + pow((metrics->be_thr - mean->be_thr) , 2.0)*(i-1)/i;

		var->drop_ratio = var->drop_ratio + pow((metrics->drop_ratio - mean->drop_ratio) , 2.0)*(i-1)/i;
		var->abort_ratio = var->abort_ratio + pow((metrics->abort_ratio - mean->abort_ratio) , 2.0)*(i-1)/i;

		//MEDIE:
		mean->sys_resp = (mean->sys_resp) + (metrics->sys_resp - mean->sys_resp)/i;
		mean->sys_thr = (mean->sys_thr) + (metrics->sys_thr - mean->sys_thr)/i;

		mean->fs_resp = (mean->fs_resp) + (metrics->fs_resp - mean->fs_resp)/i;
		mean->fs_thr = (mean->fs_thr) + (metrics->fs_thr - mean->fs_thr)/i;

		mean->be_resp = (mean->be_resp) + (metrics->be_resp - mean->be_resp)/i;
		mean->be_delay = (mean->be_delay) + (metrics->be_delay - mean->be_delay)/i;
		mean->be_thr = (mean->be_thr) + (metrics->be_thr - mean->be_thr)/i;

		mean->drop_ratio = (mean->drop_ratio) + (metrics->drop_ratio - mean->drop_ratio)/i;
		mean->abort_ratio = (mean->abort_ratio) + (metrics->abort_ratio - mean->abort_ratio)/i;


		reset_state();
		reset_metrics(metrics);

		printf("\n");
	}

	printf("System Response Time: \n");
	printf("Mean: %6.6f\n",mean->sys_resp);
	printf("Var: %6.6f\n",(var->sys_resp/(i-2)));
	printf("Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->sys_resp/(i-2))));

	printf("System Useful Throughput: \n");
	printf("Mean: %6.6f\n",mean->sys_thr);
	printf("Var: %6.6f\n",(var->sys_thr/(i-2)));
	printf("Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->sys_thr/(i-2))));

	printf("Fs Response Time: \n");
	printf("Mean: %6.6f\n",mean->fs_resp);
	printf("Var: %6.6f\n",(var->fs_resp/(i-2)));
	printf("Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->fs_resp/(i-2))));

	printf("Fs Throughput: \n");
	printf("Mean: %6.6f\n",mean->fs_thr);
	printf("Var: %6.6f\n",(var->fs_thr/(i-2)));
	printf("Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->fs_thr/(i-2))));

	printf("Be Response Time: \n");
	printf("Mean: %6.6f\n",mean->be_resp);
	printf("Var: %6.6f\n",(var->be_resp/(i-2)));
	printf("Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->be_resp/(i-2))));

	printf("Be Delay Time: \n");
	printf("Mean: %6.6f\n",mean->be_delay);
	printf("Var: %6.6f\n",(var->be_delay/(i-2)));
	printf("Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->be_delay/(i-2))));

	printf("Be Throughput: \n");
	printf("Mean: %6.6f\n",mean->be_thr);
	printf("Var: %6.6f\n",(var->be_thr/(i-2)));
	printf("Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->be_thr/(i-2))));

	printf("Drop Ratio: \n");
	printf("Mean: %6.6f\n",mean->drop_ratio);
	printf("Var: %6.6f\n",(var->drop_ratio/(i-2)));
	printf("Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->drop_ratio/(i-2))));

	printf("Abort Ratio: \n");
	printf("Mean: %6.6f\n",mean->abort_ratio);
	printf("Var: %6.6f\n",(var->abort_ratio/(i-2)));
	printf("Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->abort_ratio/(i-2))));

	/** SCRITTURA SU FILE DEL TREND TEMPORALE DELLA METRICA SELEZIONATA **/

	FILE* trend_f = fopen(choice, "a+");
	int index;

	for(index = 0 ; index < STOP_SIMULATION ; index ++)
	{
		fprintf(trend_f, "%6.6f,", (double)(trend[index]/(i-1)));
	}

	fclose(trend_f);

	destroy_metrics(&metrics);

	free(trend);

	printf("END SIMULATIONS: ALL DATA STORED\n");

	return EXIT_SUCCESS;
}
