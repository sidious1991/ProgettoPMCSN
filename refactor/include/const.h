#ifndef _CONST_
#define _CONST_

typedef enum { false, true } bool;

#define DROP_THRESHOLD 0.85
#define DROP_FLOOR 0.75

#define NUM_RUN 501

#define STOP_SIMULATION  10000
#define SAMPLING_TIME 1

#define NUM_EVENTS 5

#define NEW_SESSION_EVENT 0
#define EXIT_FE_EVENT 1
#define EXIT_BE_EVENT 2
#define EXIT_TH_EVENT 3
#define SAMPLING_EVENT 4

#define CLASS_1 0
#define CLASS_2 1

#define LOWER_EQ 5
#define UPPER_EQ 35

#define INTER_ARRIVAL_TIME 0.02857
#define FE_AVG_TIME 0.00456
#define BE_AVG_TIME 0.00117
#define TH_AVG_TIME 7

#endif
