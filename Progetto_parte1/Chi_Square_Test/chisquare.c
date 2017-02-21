#include <stdio.h>
#include <stdlib.h>  
#include <math.h>
#include "rngs.h"
#include "rvms.h"

#define K 1000 		/*It is the number of bin (K >= 1000)*/
#define N 10000		/*It is the number of variate for STREAM (N >= 10*K)*/
#define ALFA 0.05	/*Level of confidence of Chi-Square test*/
#define STREAMS 256	/*Considered STREAMS*/

double chiSquareByStream(int index){
/*Return the Chi-Square statistic based on N variates selected by STREAM index*/

	double expected = (N/K);
	double statistic = 0;

	int o[K];
	int x;
	int i;

	for(x = 0 ; x < K ; x++)
		o[x] = 0;
	
	SelectStream(index);
	
	for(i = 0 ; i < N ; i++){
		x = floor((Random())*K);
		o[x]++;
	}

	for(x = 0 ; x < K ; x++){
		statistic = statistic + (double)(pow((o[x] - expected) , 2)) / (expected);
	}

	return statistic;
}


double* chiSquare(){
/*Return all 256 Chi-Square statistics, one for STREAM*/
	double* V = (double*)malloc(STREAMS*sizeof(double));

	if(V==NULL) {
    		perror("Error in malloc\n");
    		exit(1);
  	}
		
	int i;

	for(i = 0 ; i < STREAMS ; i++)
		V[i] = 0;

	for(i = 0 ; i < STREAMS ; i++)
		V[i] = chiSquareByStream(i);

	return V;
}

double computeV1(void){
/*Return idfChisquare(K − 1, ALFA / 2)*/
	
	return idfChisquare((long)(K - 1), (double)(ALFA / 2));
}

double computeV2(void){
/*Return idfChisquare(K − 1, 1 - ALFA / 2)*/

	return idfChisquare((long)(K - 1), (double)(1 - (ALFA / 2)));
}

void destroy(double* v){
/*Free heap*/

	free(v);
}

int  writeData(void){
/*Write results of chiSquare() on data.txt"*/

	FILE* fd;
	int i;

	fd = fopen("data", "a");

	if(fd==NULL) {
    		perror("Error opening file");
    		exit(1);
  	}

	double* V = chiSquare();

	for(i = 0 ; i < STREAMS ; i++)
		fprintf(fd, "%0.2f,", V[i]);

	fprintf(fd, "%0.2f,", computeV1());
	fprintf(fd, "%0.2f", computeV2());

	destroy(V);
	fclose(fd);

	return 0;
}



int main(void) 
{
	int i = writeData();

	if(i != 0)
		return EXIT_FAILURE;
	else
		return EXIT_SUCCESS;
}

