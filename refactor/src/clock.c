#include <stdio.h>
#include <stdlib.h>
#include "clock.h"

struct clock* create_clock(double next) {

	struct clock* c = (struct clock*) malloc(sizeof(struct clock));

	if(c == NULL)
	{
		perror("Error create_clock()\n");
		exit(1);
	}

	c->current = 0;
	c->next = next;

	return c;
}

void destroy_clock(struct clock** c) {
	if(c != NULL)
	{
		free(*c);
		*c = NULL;
	}
}

struct calendar* create_calendar(double new_session_time) {
	struct calendar* c = (struct calendar*) malloc(sizeof(struct calendar));

	if(c == NULL)
	{
		perror("Error create_calendar()\n");
		exit(1);
	}

	c->events_times = (double*) malloc(NUM_EVENTS*sizeof(double));

	c->events_times[NEW_SESSION_EVENT] = new_session_time;
	c->events_times[EXIT_FE_EVENT] = 100 * STOP_SIMULATION;
	c->events_times[EXIT_BE_EVENT] = 100 * STOP_SIMULATION;
	c->events_times[EXIT_TH_EVENT] = 100 * STOP_SIMULATION;
	c->events_times[SAMPLING_EVENT] = SAMPLING_TIME;

	return c;
}

void set_new_session_time(struct calendar* c, double new_session_time) {
	if(c == NULL)
	{
		perror("Null pointer set_new_session_time()\n");
		exit(1);
	}

	c->events_times[0] = new_session_time;

}

void set_exit_fs_time(struct calendar* c, double exit_fs_time) {
	if(c == NULL)
	{
		perror("Null pointer set_exit_fs_time()\n");
		exit(1);
	}

	c->events_times[1] = exit_fs_time;

}

void set_exit_be_time(struct calendar* c, double exit_be_time) {
	if(c == NULL)
	{
		perror("Null pointer set_exit_be_time()\n");
		exit(1);
	}

	c->events_times[2] = exit_be_time;

}

void set_exit_th_time(struct calendar* c, double exit_th_time) {
	if(c == NULL)
	{
		perror("Null pointer set_exit_th_time()\n");
		exit(1);
	}

	c->events_times[3] = exit_th_time;

}

void set_sample_time(struct calendar* c, double smp_time) {
	if(c == NULL)
	{
		perror("Null pointer set_exit_th_time()\n");
		exit(1);
	}

	c->events_times[4] = smp_time;

}

void destroy_calendar(struct calendar** c) {
	if(*c != NULL)
	{
		free((*c)->events_times);

		free(*c);
		*c = NULL;
	}
}

int min(struct calendar* c) {
	int pos = 0;
	int j;

	for(j = pos+1 ; j < NUM_EVENTS ; j++)
	{
		if(c->events_times[pos] > c->events_times[j])
			pos = j;
	}

	return pos;
}
