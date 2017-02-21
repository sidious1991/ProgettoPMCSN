#ifndef _METRICS_
#define _METRICS_

struct metrics {
	double sys_resp;
	double sys_thr;

	double fe_resp;
	double fe_thr;
	double fe_area;

	double be_resp;
	double be_thr;
	double be_area;

	double drop_ratio;
	double abort_ratio;

	double total_time;
};

struct metrics* create_metrics(void);

void init_metrics(struct metrics* m);

void destroy_metrics(struct metrics** m);

#endif
