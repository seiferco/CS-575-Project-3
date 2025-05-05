#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <string>

// setting the number of threads:
#ifndef NUMT
#define NUMT		    4
#endif

// setting the number of capitals we want to try:
#ifndef NUMCAPITALS
#define NUMCAPITALS	4
#endif


// maximum iterations to allow looking for convergence:
#define MAXITERATIONS	100

// how many tries to discover the maximum performance:
#define NUMTRIES	30

#define CSV

struct city
{
	std::string	name;
	float		longitude;
	float		latitude;
	int			capitalnumber;
	float		mindistance;
};

#include "UsCities.data"

// setting the number of cities we want to try:
#define NUMCITIES 	( sizeof(Cities) / sizeof(struct city) )

struct capital
{
	std::string	name;
	float		longitude;
	float		latitude;
	float		longsum;
	float		latsum;
	int			numsum;
};


struct capital	Capitals[NUMCAPITALS];


float
Distance( int city, int capital )
{
	float dx = Cities[city].longitude - Capitals[capital].longitude;
	float dy = Cities[city].latitude  - Capitals[capital].latitude;
	return sqrtf( dx*dx + dy*dy );
}


int
main( int argc, char *argv[ ] )
{
#ifdef _OPENMP
	fprintf( stderr, "OpenMP is supported -- version = %d\n", _OPENMP );
#else
        fprintf( stderr, "No OpenMP support!\n" );
        return 1;
#endif

	for (int t = 0; t < NUMTRIES; t++) {

		omp_set_num_threads( NUMT );    // set the number of threads to use in parallelizing the for-loop:`

		// seed the capitals:
		// (this is just picking initial capital cities at uniform intervals)
		for( int k = 0; k < NUMCAPITALS; k++ )
		{
			int cityIndex = k * (NUMCITIES-1) / (NUMCAPITALS-1);
			Capitals[k].longitude = Cities[cityIndex].longitude;
			Capitals[k].latitude  = Cities[cityIndex].latitude;
		}

		double time0, time1;
		for( int n = 0;  n < MAXITERATIONS; n++ )
		{
			// reset the summations for the capitals:
			for( int k = 0; k < NUMCAPITALS; k++ )
			{
				Capitals[k].longsum = 0.;
				Capitals[k].latsum  = 0.;
				Capitals[k].numsum = 0;
			}

			time0 = omp_get_wtime( );

			// Compute distances betwwen all cities and capitals
			// Assign cities to a capital depending on shortest distance
			#pragma omp parallel for
			for( int i = 0; i < NUMCITIES; i++ )
			{
				int capitalnumber = -1;
				float mindistance = 1.e+37;

				for( int k = 0; k < NUMCAPITALS; k++ )
				{
					float dist = Distance( i, k );
					if( dist < mindistance )
					{
						mindistance = dist;
						Cities[i].mindistance = mindistance;
						Cities[i].capitalnumber = k;
					}
				}
				// Set k=closest capital to current city
				int k = Cities[i].capitalnumber;
				// this is here for the same reason as the Trapezoid noteset uses it:
				#pragma omp critical
				{
					Capitals[k].longsum += Cities[i].longitude;
					Capitals[k].latsum  += Cities[i].latitude;
					Capitals[k].numsum++;
				}
			}
			time1 = omp_get_wtime( );

			// Reassign capitals
			// get the average longitude and latitude for each capital:
			for( int k = 0; k < NUMCAPITALS; k++ )
			{
				Capitals[k].longitude = Capitals[k].longsum/Capitals[k].numsum;
				Capitals[k].latitude  = Capitals[k].latsum/Capitals[k].numsum;
			}
		}

		double megaCityCapitalsPerSecond = (double)NUMCITIES * (double)NUMCAPITALS / ( time1 - time0 ) / 1000000.;


		// figure out what actual city is closest to each capital:
		// this is the extra credit:
		for( int k = 0; k < NUMCAPITALS; k++ )
		{
			float min_dist = 1.e+37;
			int closest_city_idx;

			// Iterate through every city and compute distances between the capitals (Find smallest)
			for (int i = 0; i < NUMCITIES; i++) {

				float curr_dist = Distance(i, k);
				if (curr_dist < min_dist) {
					closest_city_idx = i;
					Capitals[k].name = Cities[closest_city_idx].name;
					min_dist = curr_dist;
				}
			}

		}


		// print the longitude-latitude of each new capital city:
		// you only need to do this once per some number of NUMCAPITALS -- do it for the 1-thread version:
		if( NUMT == 1 )
		{
			for( int k = 0; k < NUMCAPITALS; k++ )
			{
				// fprintf( stderr, "\t%3d:  %8.2f , %8.2f\n", k, Capitals[k].longitude, Capitals[k].latitude );

				//if you did the extra credit, use this fprintf instead:
				fprintf( stderr, "\t%3d:  %8.2f , %8.2f , %s\n", k, Capitals[k].longitude, Capitals[k].latitude, Capitals[k].name.c_str() );
			}
		}

	#ifdef CSV
	printf("%d,%d,%d,%.3lf\n", NUMT, NUMCITIES, NUMCAPITALS, megaCityCapitalsPerSecond);
	#else
			fprintf(stderr, "%2d threads : %4d cities ; %4d capitals; megatrials/sec = %8.3lf\n",
					NUMT, NUMCITIES, NUMCAPITALS, megaCityCapitalsPerSecond );
	#endif

	}

}