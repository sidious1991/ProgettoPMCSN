#include <stdio.h>
#include <stdlib.h>
#include "metrics.h"

struct metrics* create_metrics(void) {
	struct metrics* m = (struct metrics*) malloc(sizeof(struct metrics));

	if(m == NULL)
	{
		perror("Error in malloc create_metrics()\n");
		exit(1);
	}

	init_metrics(m);

	return m;
}

void init_metrics(struct metrics* m) {
	m->sys_resp = 0.0;
	m->sys_thr = 0.0;

	m->fe_resp = 0.0;
	m->fe_thr = 0.0;
	m->fe_area = 0.0;

	m->be_resp = 0.0;
	m->be_thr = 0.0;
	m->be_area = 0.0;

	m->drop_ratio = 0.0;
	m->abort_ratio = 0.0;

	m->total_time = 0.0;
}

void destroy_metrics(struct metrics** m) {
	if(*m != NULL)
	{
		free(*m);
		*m = NULL;
	}
}
