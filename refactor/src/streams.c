#include "rngs.h"
#include "rvgs.h"
#include "rvms.h"
#include "streams.h"

int GetSessionLength(void) {
  SelectStream(0);
  return Equilikely(LOWER_EQ, UPPER_EQ);
}

double GetInterArrival(void) {
  SelectStream(1);
  return Exponential((double)INTER_ARRIVAL_TIME);
}

double GetEndFe(void) {
  SelectStream(2);
  return Exponential((double)FE_AVG_TIME);
}

double GetEndBe(void) {
  SelectStream(3);
  return Exponential((double)BE_AVG_TIME);
}

double GetEndTh(void) {
  SelectStream(4);
  return Exponential((double)TH_AVG_TIME);
}
