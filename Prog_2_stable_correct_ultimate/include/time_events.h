#ifndef _TIME_EVENTS_
#define _TIME_EVENTS_

typedef int bool;
#define true 1
#define false 0

#define STOP_SIMULATION  10000	//Tempo di fine simulazione (oltre non entrano nuove sessioni e vengono servite le restanti sessioni attive)
#define T_SAMPLE 1	//Periodo di sampling

#define EVENTS_N 5

#define NEW_SESSION_INDEX 0
#define EXIT_FS_INDEX 1
#define EXIT_BE_INDEX 2
#define EXIT_TH_INDEX 3
#define SAMPLING_INDEX 4

/*----Modella il clock di simulazione----*/
struct clock
{
	double current;
	double next;
};

struct clock* create_clock(double next);
void destroy_clock(struct clock** c);


/*----Modella una struttura che mantiene l'istante di prossima occorrenza per ogni tipo di evento----*/
struct calendar
{
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
