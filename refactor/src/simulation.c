#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "const.h"
#include "rngs.h"
#include "rvgs.h"
#include "rvms.h"
#include "structs.h"
#include "clock.h"
#include "streams.h"
#include "metrics.h"

double* values;
char string[21];

bool omm = false;
bool drop = false;

long session_fe = 0;
int state_fe = 0;
long session_be = 0;
int state_be = 0;
long session_th = 0;

long total_generated_sessions = 0;
long total_completed_sessions = 0;
long total_dropped_sessions = 0;
long total_processed_requests = 0;
long total_aborted_requests = 0;

long num_ended_req_fe = 0;
long num_ended_req_be = 0;
long num_entered_be_serv = 0;

void init_state(void) {
	drop = false;
	session_fe = 0;
	state_fe = 0;
	session_be = 0;
	state_be = 0;
	session_th = 0;
	total_generated_sessions = 0;
	total_completed_sessions = 0;
	total_dropped_sessions = 0;
	total_processed_requests = 0;
	total_aborted_requests = 0;
	num_ended_req_fe = 0;
	num_ended_req_be = 0;
	num_entered_be_serv = 0;
}

void handle_new_session_event(struct metrics* metrics, struct node *n, struct clock* cl, struct calendar* c, struct frontend* fe) {

	total_generated_sessions++;

	if(drop == false)
	{
		n = create_node();
		n->length = GetSessionLength();

		if(n->length <= 20) {
			n->class = CLASS_1;
		} else {
			n->class = CLASS_2;
		}

		n->arrival_FE = cl->current;
		n->sessionStart = cl->current;
		add_new_req_frontend(fe, n);
		state_fe = 1;
		session_fe = fe->queue1->size + fe->queue2->size;

		if(fe->queue1->size == 0 && fe->queue2->size == 0) {
			c->events_times[EXIT_FE_EVENT] = cl->current + GetEndFe();
			n->arrival_BE = c->events_times[EXIT_FE_EVENT];
		}

	}

	else
	{
		total_dropped_sessions++;
	}

	c->events_times[NEW_SESSION_EVENT] = cl->current + GetInterArrival();
	if(c->events_times[NEW_SESSION_EVENT] >= STOP_SIMULATION) {
		c->events_times[NEW_SESSION_EVENT] = 100 * STOP_SIMULATION;
	}

}

void handle_exit_fe_event(struct metrics* metrics, struct node *n, struct clock* cl, struct calendar* c, struct frontend* fe, struct server* be) {
	n = update_worker_node_frontend(fe);

	if(fe->work != NULL) {
		c->events_times[EXIT_FE_EVENT] = cl->current + GetEndFe();
		fe->work->arrival_BE = c->events_times[EXIT_FE_EVENT];
	} else {
		c->events_times[EXIT_FE_EVENT] = 100 * STOP_SIMULATION;
		state_fe = 0;
	}

	session_fe = fe->queue1->size + fe->queue2->size;
	num_ended_req_fe++;

	(metrics->fe_resp) = (metrics->fe_resp) + ((n->arrival_BE - n->arrival_FE) - (metrics->fe_resp))/num_ended_req_fe;

	add_new_req(be, n);

	state_be = 1;
	session_be = be->queue->size;

	if(be->queue->size == 0) {
		c->events_times[EXIT_BE_EVENT] = cl->current + GetEndBe();
		n->ended_BE = c->events_times[EXIT_BE_EVENT];

		num_entered_be_serv++;

	}
}

void handle_exit_be_event(struct metrics* metrics, struct node *n, struct clock* cl, struct calendar* c, struct server* be, struct list* think_node) {
	n = update_worker_node(be);
	if(be->work != NULL) {
		c->events_times[EXIT_BE_EVENT] = cl->current + GetEndBe();
		be->work->ended_BE = c->events_times[EXIT_BE_EVENT];
		num_entered_be_serv++;
	} else {
		c->events_times[EXIT_BE_EVENT] = 100 * STOP_SIMULATION;
		state_be = 0;
	}

	session_be = be->queue->size;
	num_ended_req_be++;
	total_processed_requests++;

	n->length--;

	metrics->be_thr = (num_ended_req_be)/(cl->current);

	(metrics->be_resp) = (metrics->be_resp) + ((n->ended_BE - n->arrival_BE) - (metrics->be_resp))/num_ended_req_be;

	(metrics->sys_resp) = (metrics->sys_resp) + ((n->ended_BE - n->arrival_FE) - (metrics->sys_resp))/num_ended_req_be;

	if(n->length == 0) {
		n->sessionEnded = cl->current;
		total_completed_sessions++;
		metrics->sys_thr = (total_completed_sessions/(cl->current));
		destroy_node(&n);
	} else {

		n->thinkTime = cl->current + GetEndTh();
		add_think_node(think_node, n);
		session_th = think_node->size;
		c->events_times[EXIT_TH_EVENT] = think_node->head->thinkTime;
	}
}

void handle_exit_th_event(struct metrics* metrics, struct node *n, struct clock* cl, struct calendar* c, struct frontend* fe, struct list* think_node) {
	n = fetch_node(think_node);

	if(think_node->size == 0) {
		c->events_times[EXIT_TH_EVENT] = 100 * STOP_SIMULATION;
	} else {
		c->events_times[EXIT_TH_EVENT] = think_node->head->thinkTime;
	}

	n->arrival_FE = cl->current;

	add_new_req_frontend(fe, n);
	state_fe = 1;
	session_fe = fe->queue1->size + fe->queue2->size;

	if(fe->queue1->size == 0 && fe->queue2->size == 0) {
		c->events_times[EXIT_FE_EVENT] = cl->current + GetEndFe();
		n->arrival_BE = c->events_times[EXIT_FE_EVENT];
	}

	session_th = think_node->size;
}

void handle_sampling_event(struct metrics* metrics, struct clock* cl, struct calendar* c) {
	if(cl->current <= STOP_SIMULATION)
	{
			if(strcmp(string,"sys_resp") == 0)
				values[(int)(cl->current - 1)] += metrics->sys_resp;

			else if (strcmp(string,"sys_thr") == 0)
				values[(int)(cl->current - 1)] += metrics->sys_thr;

			else if (strcmp(string,"fe_resp") == 0)
				values[(int)(cl->current - 1)] += metrics->fe_resp;

			else if (strcmp(string,"fe_thr") == 0)
				values[(int)(cl->current - 1)] += metrics->fe_thr;

			else if (strcmp(string,"fe_area") == 0)
				values[(int)(cl->current - 1)] += (metrics->fe_area);

			else if (strcmp(string,"be_resp") == 0)
				values[(int)(cl->current - 1)] += metrics->be_resp;

			else if (strcmp(string,"be_thr") == 0)
				values[(int)(cl->current - 1)] += metrics->be_thr;

			else if (strcmp(string,"drop_ratio") == 0)
				values[(int)(cl->current - 1)] += metrics->drop_ratio;
			else if (strcmp(string,"abort_ratio") == 0)
				values[(int)(cl->current - 1)] += metrics->abort_ratio;

	}

	set_sample_time(c, cl->current + SAMPLING_TIME);
}

void update_metrics(struct metrics* metrics, struct clock* cl, struct area* area_fe, struct area* area_be) {
	metrics->sys_thr = (total_completed_sessions/(cl->current));
	metrics->fe_area = (area_fe->service)/(cl->current);
	metrics->fe_thr = (num_ended_req_fe)/(cl->current);
	metrics->be_area = (area_be->service)/(cl->current);
	metrics->be_thr = (num_ended_req_be)/(cl->current);
	metrics->drop_ratio = ((double)(total_dropped_sessions)/(double)(total_generated_sessions));
	metrics->abort_ratio = ((double)(total_aborted_requests)/(double)(total_processed_requests));
}

void update_measures(struct metrics* measures, struct metrics* metrics) {
	measures->sys_resp = metrics->sys_resp;
	measures->sys_thr = metrics->sys_thr;
	measures->fe_resp = metrics->fe_resp;
	measures->fe_thr = metrics->fe_thr;
	measures->fe_area = metrics->fe_area;
	measures->be_resp = metrics->be_resp;
	measures->be_thr = metrics->be_thr;
	measures->drop_ratio = metrics->drop_ratio;
	measures->abort_ratio = metrics->abort_ratio;
}

void simulation(struct metrics* measures) {

	PlantSeeds(123456789);

	double previous;

	struct metrics* metrics = create_metrics();

	double new_session_time = GetInterArrival();
	struct calendar* c = create_calendar(new_session_time);

	struct frontend* fe = create_frontend();
	struct area* area_fe = create_area();
	struct server* be = create_server();
	struct area* area_be = create_area();

	struct list* think_node = create_list();

	struct clock* cl = create_clock(c->events_times[min(c)]);
	int index = min(c);

	struct node *n = malloc(sizeof(intptr_t));

	while(c->events_times[NEW_SESSION_EVENT] < STOP_SIMULATION || state_fe + state_be + session_th > 0)
	{
		previous = cl->current;
		cl->current = cl->next;

		update_service(area_fe, (cl->current - previous)*state_fe);
		update_queue(area_fe, (cl->current - previous)*(session_fe));
		update_node(area_fe, (cl->current - previous)*(session_fe + state_fe));

		update_service(area_be ,(cl->current - previous)*state_be);
		update_queue(area_be, (cl->current - previous)*(session_be));
		update_node(area_be, (cl->current - previous)*(session_be + state_be));

		update_metrics(metrics, cl, area_fe, area_be);

		if(cl->current == STOP_SIMULATION)
		{
			update_measures(measures, metrics);
		}

		if(omm == true)
		{
			if (drop == false && metrics->fe_area >= DROP_THRESHOLD)
				drop = true;

			else if(drop == true && metrics->fe_area <= DROP_FLOOR)
				drop = false;
		}

		switch (index) {

			case NEW_SESSION_EVENT:
				handle_new_session_event(metrics, n, cl, c, fe);
				break;

			case EXIT_FE_EVENT:
				handle_exit_fe_event(metrics, n, cl, c, fe, be);
				break;

			case EXIT_BE_EVENT:
				handle_exit_be_event(metrics, n, cl, c, be, think_node);
				break;

			case EXIT_TH_EVENT:
				handle_exit_th_event(metrics, n, cl, c, fe, think_node);
				break;

			case SAMPLING_EVENT:
				handle_sampling_event(metrics, cl, c);
				break;

			default:
				printf("Bad event!\n");
		}

		cl->next = c->events_times[min(c)];
		index = min(c);

	}

	measures->total_time = cl->current;

	destroy_calendar(&c);
	destroy_clock(&cl);
	destroy_frontend(&fe);
	destroy_server(&be);
	destroy_list(&think_node);
	destroy_area(&area_fe);
	destroy_area(&area_be);
	destroy_metrics(&metrics);

	free(n);
}

double confidence_interval(int n, double s) {
	double t = idfStudent(n-1, 0.975);
	double sqnum = sqrt((double) n);
	double sqs = sqrt(s);

	return (t * sqs) / sqnum;
}

void write_files(struct metrics* mean, struct metrics* var) {
	FILE* values_f = fopen(string, "a+");
	
	int index;
	int i = NUM_RUN;

	for(index = 0 ; index < STOP_SIMULATION ; index++)
	{
		fprintf(values_f, "%6.6f,", (double)(values[index]/(i-1)));
	}

	fclose(values_f);

	FILE* stats_f = fopen("stats", "a+");

	fprintf(stats_f, "System Response Time\n");
	fprintf(stats_f, "Mean: %6.6f\n",mean->sys_resp);
	fprintf(stats_f, "Variance: %6.6f\n",(var->sys_resp/(i-2)));
	fprintf(stats_f, "Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->sys_resp/(i-2))));

	fprintf(stats_f, "System Useful Throughput\n");
	fprintf(stats_f, "Mean: %6.6f\n",mean->sys_thr);
	fprintf(stats_f, "Variance: %6.6f\n",(var->sys_thr/(i-2)));
	fprintf(stats_f, "Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->sys_thr/(i-2))));

	fprintf(stats_f, "Frontend Response Time\n");
	fprintf(stats_f, "Mean: %6.6f\n",mean->fe_resp);
	fprintf(stats_f, "Variance: %6.6f\n",(var->fe_resp/(i-2)));
	fprintf(stats_f, "Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->fe_resp/(i-2))));

	fprintf(stats_f, "Frontend Throughput\n");
	fprintf(stats_f, "Mean: %6.6f\n",mean->fe_thr);
	fprintf(stats_f, "Variance: %6.6f\n",(var->fe_thr/(i-2)));
	fprintf(stats_f, "Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->fe_thr/(i-2))));

	fprintf(stats_f, "Frontend Utilization\n");
	fprintf(stats_f, "Mean: %6.6f\n",mean->fe_area);
	fprintf(stats_f, "Variance: %6.6f\n",(var->fe_area/(i-2)));
	fprintf(stats_f, "Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->fe_area/(i-2))));

	fprintf(stats_f, "Backend Response Time\n");
	fprintf(stats_f, "Mean: %6.6f\n",mean->be_resp);
	fprintf(stats_f, "Variance: %6.6f\n",(var->be_resp/(i-2)));
	fprintf(stats_f, "Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->be_resp/(i-2))));

	fprintf(stats_f, "Backend Throughput\n");
	fprintf(stats_f, "Mean: %6.6f\n",mean->be_thr);
	fprintf(stats_f, "Variance: %6.6f\n",(var->be_thr/(i-2)));
	fprintf(stats_f, "Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->be_thr/(i-2))));

	fprintf(stats_f, "Drop Ratio\n");
	fprintf(stats_f, "Mean: %6.6f\n",mean->drop_ratio);
	fprintf(stats_f, "Variance: %6.6f\n",(var->drop_ratio/(i-2)));
	fprintf(stats_f, "Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->drop_ratio/(i-2))));

	fprintf(stats_f, "Abort Ratio\n");
	fprintf(stats_f, "Mean: %6.6f\n",mean->abort_ratio);
	fprintf(stats_f, "Variance: %6.6f\n",(var->abort_ratio/(i-2)));
	fprintf(stats_f, "Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->abort_ratio/(i-2))));

	fprintf(stats_f, "Total Time\n");
	fprintf(stats_f, "Mean: %6.6f\n",mean->total_time);
	fprintf(stats_f, "Variance: %6.6f\n",(var->total_time/(i-2)));
	fprintf(stats_f, "Confidence Interval: %6.6f\n\n", confidence_interval(i-1 , (var->total_time/(i-2))));

	fclose(stats_f);

}

void update_variance(int i,struct metrics* metrics, struct metrics* mean, struct metrics* var) {
	var->sys_resp = var->sys_resp + pow((metrics->sys_resp - mean->sys_resp), 2.0)*(i-1)/i;
	var->sys_thr = var->sys_thr + pow((metrics->sys_thr - mean->sys_thr), 2.0)*(i-1)/i;
	var->fe_resp = var->fe_resp + pow((metrics->fe_resp - mean->fe_resp), 2.0)*(i-1)/i;
	var->fe_thr = var->fe_thr + pow((metrics->fe_thr - mean->fe_thr), 2.0)*(i-1)/i;
	var->fe_area = var->fe_area + pow((metrics->fe_area - mean->fe_area), 2.0)*(i-1)/i;
	var->be_resp = var->be_resp + pow((metrics->be_resp - mean->be_resp), 2.0)*(i-1)/i;
	var->be_thr = var->be_thr + pow((metrics->be_thr - mean->be_thr), 2.0)*(i-1)/i;
	var->drop_ratio = var->drop_ratio + pow((metrics->drop_ratio - mean->drop_ratio), 2.0)*(i-1)/i;
	var->abort_ratio = var->abort_ratio + pow((metrics->abort_ratio - mean->abort_ratio), 2.0)*(i-1)/i;
	var->total_time = var->total_time + pow((metrics->total_time - mean->total_time), 2.0)*(i-1)/i;
}

void update_mean(int i,struct metrics* metrics, struct metrics* mean, struct metrics* var) {
	mean->sys_resp = (mean->sys_resp) + (metrics->sys_resp - mean->sys_resp)/i;
	mean->sys_thr = (mean->sys_thr) + (metrics->sys_thr - mean->sys_thr)/i;
	mean->fe_resp = (mean->fe_resp) + (metrics->fe_resp - mean->fe_resp)/i;
	mean->fe_thr = (mean->fe_thr) + (metrics->fe_thr - mean->fe_thr)/i;
	mean->fe_area = (mean->fe_area) + (metrics->fe_area - mean->fe_area)/i;
	mean->be_resp = (mean->be_resp) + (metrics->be_resp - mean->be_resp)/i;
	mean->be_thr = (mean->be_thr) + (metrics->be_thr - mean->be_thr)/i;
	mean->drop_ratio = (mean->drop_ratio) + (metrics->drop_ratio - mean->drop_ratio)/i;
	mean->abort_ratio = (mean->abort_ratio) + (metrics->abort_ratio - mean->abort_ratio)/i;
	mean->total_time = (mean->total_time) + (metrics->total_time - mean->total_time)/i;
}

int main(void) {
	printf("Enter:\n0 - OMM Deactivated\n1 - OMM Activated\n");

	scanf("%d",&omm);

	printf("\nEnter the metric to measure:\n- sys_resp\n- sys_thr\n- fe_resp\n- fe_thr\n- fe_area\n- be_resp\n- be_thr\n- drop_ratio\n- abort_ratio\n");

	scanf("%s", string);

	printf("\n");

	struct metrics* metrics = create_metrics();
	struct metrics* mean = create_metrics();
	struct metrics* var = create_metrics();

	values = (double*) malloc(10000*sizeof(double));

	if(values == NULL)
	{
		perror("Error malloc()\n");
	}

	memset(values, 0.0, sizeof(double)*10000);

	int i;

	for(i = 1 ; i < NUM_RUN; i++)
	{
		printf("Run %d\n", i);

		simulation(metrics);

		update_variance(i, metrics, mean, var);
		update_mean(i, metrics, mean, var);

		init_state();
		init_metrics(metrics);

	}

	write_files(mean, var);
	
	destroy_metrics(&metrics);
	destroy_metrics(&mean);
	destroy_metrics(&var);

	free(values);

	printf("End Simulation\n");

	return EXIT_SUCCESS;
}
