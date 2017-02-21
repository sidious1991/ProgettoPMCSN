#include <stdio.h>
#include <stdlib.h>
#include "time_events.h"

/*----CLOCK----*/

struct clock* create_clock(double next)
{
	/*Crea un'istanza di clock*/

	struct clock* c = (struct clock*)malloc(sizeof(struct clock));

	if(c == NULL)
	{
		perror("Error in malloc of create_clock()!\n");
		exit(1);
	}

	c->current = 0; // Tempo corrente inizio simulazione
	c->next = next; // Tempo primo evento prossimo

	return c;
}

void destroy_clock(struct clock** c)
{
	/*Rimuove dallo heap un clock*/

	if(c != NULL)
	{
		free(*c);
		*c = NULL;
	}
}

/*----CALENDAR----*/

struct calendar* create_calendar(double new_session_time)
{
	/*Crea un'istanza di calendar*/

	struct calendar* c = (struct calendar*)malloc(sizeof(struct calendar));

	if(c == NULL)
	{
		perror("Error in malloc of create_calendar()!\n");
		exit(1);
	}

	c->events_times = (double*)malloc(EVENTS_N*sizeof(double)); // Array dei tempi per ciascun tipo di evento

	c->events_times[NEW_SESSION_INDEX] = new_session_time;   // new_sessione
	c->events_times[EXIT_FS_INDEX] = 100*STOP_SIMULATION; // exit_fs
	c->events_times[EXIT_BE_INDEX] = 100*STOP_SIMULATION; // exit_be
	c->events_times[EXIT_TH_INDEX] = 100*STOP_SIMULATION; // exit_th
	c->events_times[SAMPLING_INDEX] = T_SAMPLE; // sample

	return c;
}

void set_new_session_time(struct calendar* c, double new_session_time)
{
	/*Setta l'istante dell'arrivo della prossima nuova sessione (tempo assoluto)*/

	if(c == NULL)
	{
		perror("Null pointers in set_new_session_time() parameters!\n");
		exit(1);
	}

	c->events_times[0] = new_session_time;

}

void set_exit_fs_time(struct calendar* c, double exit_fs_time)
{
	/*Setta l'istante prossimo di uscita di una richiesta dal fs (tempo assoluto)*/

	if(c == NULL)
	{
		perror("Null pointers in set_exit_fs_time() parameters!\n");
		exit(1);
	}

	c->events_times[1] = exit_fs_time;

}

void set_exit_be_time(struct calendar* c, double exit_be_time)
{
	/*Setta l'istante prossimo di uscita di una richiesta dal be (tempo assoluto)*/

	if(c == NULL)
	{
		perror("Null pointers in set_exit_be_time() parameters!\n");
		exit(1);
	}

	c->events_times[2] = exit_be_time;

}

void set_exit_th_time(struct calendar* c, double exit_th_time)
{
	/*Setta l'istante prossimo di uscita di una richiesta dal nodo di think (tempo assoluto)*/

	if(c == NULL)
	{
		perror("Null pointers in set_exit_th_time() parameters!\n");
		exit(1);
	}

	c->events_times[3] = exit_th_time;

}

void set_sample_time(struct calendar* c, double smp_time)
{
	/*Setta l'istante prossimo di sampling (tempo assoluto)*/

	if(c == NULL)
	{
		perror("Null pointers in set_exit_th_time() parameters!\n");
		exit(1);
	}

	c->events_times[4] = smp_time;

}

void destroy_calendar(struct calendar** c)
{
	/*Rimuove dallo heap il calendar*/

	if(*c != NULL)
	{
		free((*c)->events_times);

		free(*c);
		*c = NULL;
	}
}

int min(struct calendar* c)
{
	/* Restituisce l'indice dell'elemento minimo di events_times */

	int pos = 0;
	int j;

	for(j = pos+1 ; j < EVENTS_N ; j++)
	{
		if(c->events_times[pos] > c->events_times[j])
			pos = j;
	}

	return pos;
}

/*
int main(void)
{
	struct calendar* c = create_calendar(3);
	set_exit_fs_time(c, 2.3);
	set_exit_be_time(c, 1.3);
	set_exit_th_time(c, 0.4);

	printf("%f\n",c->events_times[min(c)]);

	return EXIT_SUCCESS;
}
*/
