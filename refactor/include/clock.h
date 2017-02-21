#ifndef _CLOCK_
#define _CLOCK_

#include "const.h"

struct clock {
	double current;
	double next;
};

struct clock* create_clock(double next);
void destroy_clock(struct clock** c);

struct calendar {
	double* events_times;
};

struct calendar* create_calendar(double new_session_time);
void set_new_session_time(struct calendar* c, double new_session_time);
void set_exit_fs_time(struct calendar* c, double exit_fs_time);
void set_exit_be_time(struct calendar* c, double exit_be_time);
void set_exit_th_time(struct calendar* c, double exit_th_time);
void set_sample_time(struct calendar* c, double smp_time);
void destroy_calendar(struct calendar** c);

int min(struct calendar* c);

#endif
