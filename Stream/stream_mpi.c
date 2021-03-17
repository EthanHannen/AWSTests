/*-----------------------------------------------------------------------*/
/* Program: Stream                                                       */
/* Revision: $Id: stream.c,v 5.8 2007/02/19 23:57:39 mccalpin Exp mccalpin $ */
/* Original code developed by John D. McCalpin                           */
/* Programmers: John D. McCalpin                                         */
/*              Joe R. Zagar                                             */
/*                                                                       */
/* This program measures memory transfer rates in MB/s for simple        */
/* computational kernels coded in C.                                     */
/*-----------------------------------------------------------------------*/
/* Copyright 1991-2005: John D. McCalpin                                 */
/*-----------------------------------------------------------------------*/
/* License:                                                              */
/*  1. You are free to use this program and/or to redistribute           */
/*     this program.                                                     */
/*  2. You are free to modify this program for your own use,             */
/*     including commercial use, subject to the publication              */
/*     restrictions in item 3.                                           */
/*  3. You are free to publish results obtained from running this        */
/*     program, or from works that you derive from this program,         */
/*     with the following limitations:                                   */
/*     3a. In order to be referred to as "STREAM benchmark results",     */
/*         published results must be in conformance to the STREAM        */
/*         Run Rules, (briefly reviewed below) published at              */
/*         http://www.cs.virginia.edu/stream/ref.html                    */
/*         and incorporated herein by reference.                         */
/*         As the copyright holder, John McCalpin retains the            */
/*         right to determine conformity with the Run Rules.             */
/*     3b. Results based on modified source code or on runs not in       */
/*         accordance with the STREAM Run Rules must be clearly          */
/*         labelled whenever they are published.  Examples of            */
/*         proper labelling include:                                     */
/*         "tuned STREAM benchmark results"                              */
/*         "based on a variant of the STREAM benchmark code"             */
/*         Other comparable, clear and reasonable labelling is           */
/*         acceptable.                                                   */
/*     3c. Submission of results to the STREAM benchmark web site        */
/*         is encouraged, but not required.                              */
/*  4. Use of this program or creation of derived works based on this    */
/*     program constitutes acceptance of these licensing restrictions.   */
/*  5. Absolutely no warranty is expressed or implied.                   */
/*-----------------------------------------------------------------------*/
/*                            MPI VERSION
 *
 *  Latest Modification: Oct 4, 2007 (M. Siegert)
 *  Based on STREAM version 5.8, 2007/02/19
 *
 * This version has been shown to work under the MPI environment
 * under Linux with MPICH2 --- comments or suggestions on how to improve
 * portability are welcome!    mailto:john@mccalpin.com
 * 
 * This program measures memory transfer rates in MB/s for simple
 * computational kernels coded in C.
 * The intent is to demonstrate the extent to which ordinary user
 * code can exploit the main memory bandwidth of the system under
 * test.
 *=========================================================================*/
# include <stdio.h>
# include <math.h>
# include <float.h>
# include <limits.h>
# include <sys/time.h>
# include <stdlib.h>

/* INSTRUCTIONS:
 *
 *      1) Stream requires a good bit of memory to run.  Adjust the
 *          value of 'N' (below) to give a 'timing calibration' of 
 *          at least 20 clock-ticks.  This will provide rate estimates
 *          that should be good to about 5% precision.
 */

# define N      2000000
# define NTIMES 10
# define OFFSET 0
static long VectorSize; /* comment out this line and uncomment next next to
                           allocate arrays statically */
/* # define VectorSize N */

/*      3) Compile the code with full optimization.  Many compilers
 *         generate unreasonably bad code before the optimizer tightens
 *         things up.  If the results are unreasonably good, on the
 *         other hand, the optimizer might be too smart for me!
 *
 *         Try compiling with:
 *               cc -O stream_mpi.c -o stream
 *
 *         This is known to work on Cray, SGI, IBM, and Sun machines.
 *      
 *         The MPI version can be compiled using something like
 *               mpicc -DPARALLEL_MPI -O stream_mpi.c -o stream_mpi
 *
 *      4) Mail the results to mccalpin@cs.virginia.edu
 *         Be sure to include:
 *              a) computer hardware model number and software revision
 *              b) the compiler flags
 *              c) all of the output from the test case.
 * Thanks!
 *
 */

# define HLINE "-------------------------------------------------------------\n"

# ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MAX
# define MAX(x,y) ((x)>(y)?(x):(y))
# endif

#define XMALLOC(t,s) ((t*)malloc(sizeof(t)*(s)))

#ifdef VectorSize
static double	a[N+OFFSET],
		b[N+OFFSET],
		c[N+OFFSET];
#else
static double *a, *b, *c;
#endif

static double   avgtime[4] = {0}, maxtime[4] = {0},
                mintime[4] = {FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX};

static char     *label[4] = {"Copy:      ", "Scale:     ",
    "Add:       ", "Triad:     "};

static double   bytes[4] = {
    2 * sizeof(double),
    2 * sizeof(double),
    3 * sizeof(double),
    3 * sizeof(double),
    };

extern void checkSTREAMresults();
#ifdef TUNED
extern void tuned_STREAM_Copy();
extern void tuned_STREAM_Scale(double scalar);
extern void tuned_STREAM_Add();
extern void tuned_STREAM_Triad(double scalar);
#endif
#ifdef _OPENMP
extern int omp_get_num_threads();
#endif

#ifdef PARALLEL_MPI
#include <mpi.h>
#define mysecond MPI_Wtime
#else
#define mysecond wtime
double wtime();
#endif

int stream(double *copyMBs, double *scaleMBs, double *addMBs, double *triadMBs,
           int *failure) {
    int                 quantum, checktick();
    int                 BytesPerWord;
    register int        j, k;
    double              scalar, t, times[4][NTIMES];
    double MBs = 1048576.0, curMBs;
    int myRank, doIO = 0;

#ifndef VectorSize
    a = XMALLOC( double, VectorSize );
    b = XMALLOC( double, VectorSize );
    c = XMALLOC( double, VectorSize );

    if (!a || !b || !c) {
      if (c) free(c);
      if (b) free(b);
      if (a) free(a);
      if (doIO) {
        printf( "Failed to allocate memory (%d).\n", VectorSize );
      }
      return 1;
    }
#endif

#ifdef PARALLEL_MPI
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    if (myRank == 0) doIO = 1;
#else
    doIO = 1;
#endif

    /* --- SETUP --- determine precision and check timing --- */

    if (doIO) {
       printf(HLINE);
       printf("STREAM version $Revision: 5.8 $\n");
       printf(HLINE);
       BytesPerWord = sizeof(double);
       printf("This system uses %d bytes per DOUBLE PRECISION word.\n",
              BytesPerWord);

       printf(HLINE);
       printf("Array size = %d, Offset = %d\n" , VectorSize, OFFSET);
       printf("Total memory required = %.1f MB.\n",
              (3.0 * BytesPerWord) * ( (double) VectorSize / 1048576.0));
       printf("Each test is run %d times, but only\n", NTIMES);
       printf("the *best* time for each is used.\n");

#ifdef _OPENMP
       printf(HLINE);
#pragma omp parallel 
       {
#pragma omp master
           {
            k = omp_get_num_threads();
	    printf ("Number of Threads requested = %i\n",k);
           }
       }
#endif

       printf(HLINE);
#pragma omp parallel
       {
       printf ("Printing one line per active thread....\n");
       }
    }

    /* Get initial value for system clock. */
#pragma omp parallel for
    for (j=0; j<VectorSize; j++) {
	a[j] = 1.0;
	b[j] = 2.0;
	c[j] = 0.0;
	}

    if (doIO) printf(HLINE);

    if  ( (quantum = checktick()) >= 1) 
	if (doIO) printf("Your clock granularity/precision appears to be "
	                  "be %d microseconds.\n", quantum);
    else {
	if (doIO) printf("Your clock granularity appears to be "
	                  "less than one microsecond.\n");
	quantum = 1;
    }

    t = mysecond();
#pragma omp parallel for
    for (j = 0; j < VectorSize; j++)
	a[j] = 2.0E0 * a[j];
    t = 1.0E6 * (mysecond() - t);

    if (doIO) {
       printf("Each test below will take on the order"
              " of %d microseconds.\n", (int) t  );
       printf("   (= %d clock ticks)\n", (int) (t/quantum) );
       printf("Increase the size of the arrays if this shows that\n");
       printf("you are not getting at least 20 clock ticks per test.\n");

       printf(HLINE);

       printf("WARNING -- The above is only a rough guideline.\n");
       printf("For best results, please be sure you know the\n");
       printf("precision of your system timer.\n");
       printf(HLINE);
    }
    
    /*	--- MAIN LOOP --- repeat test cases NTIMES times --- */

    scalar = 3.0;
    for (k=0; k<NTIMES; k++)
	{
	times[0][k] = mysecond();
#ifdef TUNED
        tuned_STREAM_Copy();
#else
#pragma omp parallel for
	for (j=0; j<VectorSize; j++)
	    c[j] = a[j];
#endif
	times[0][k] = mysecond() - times[0][k];
	
	times[1][k] = mysecond();
#ifdef TUNED
        tuned_STREAM_Scale(scalar);
#else
#pragma omp parallel for
	for (j=0; j<VectorSize; j++)
	    b[j] = scalar*c[j];
#endif
	times[1][k] = mysecond() - times[1][k];
	
	times[2][k] = mysecond();
#ifdef TUNED
        tuned_STREAM_Add();
#else
#pragma omp parallel for
	for (j=0; j<VectorSize; j++)
	    c[j] = a[j]+b[j];
#endif
	times[2][k] = mysecond() - times[2][k];
	
	times[3][k] = mysecond();
#ifdef TUNED
        tuned_STREAM_Triad(scalar);
#else
#pragma omp parallel for
	for (j=0; j<VectorSize; j++)
	    a[j] = b[j]+scalar*c[j];
#endif
	times[3][k] = mysecond() - times[3][k];
	}

    /*	--- SUMMARY --- */

    for (k=1; k<NTIMES; k++) /* note -- skip first iteration */
	{
	for (j=0; j<4; j++)
	    {
	    avgtime[j] = avgtime[j] + times[j][k];
	    mintime[j] = MIN(mintime[j], times[j][k]);
	    maxtime[j] = MAX(maxtime[j], times[j][k]);
	    }
	}
    if (doIO)
       printf("Function      Rate (MB/s)   Avg time     Min time     Max time\n");
    for (j=0; j<4; j++) {
       avgtime[j] = avgtime[j]/(double)(NTIMES-1);

       /* make sure no division by zero */
       curMBs = (mintime[j] > 0.0 ? 1.0 / mintime[j] : -1.0);
       curMBs *= 1e-6 * bytes[j] * VectorSize;
       if (doIO) printf("%s%11.4f  %11.4f  %11.4f  %11.4f\n", label[j],
                        curMBs,
                        avgtime[j],
                        mintime[j],
                        maxtime[j]);
       switch (j) {
          case 0: *copyMBs = curMBs; break;
          case 1: *scaleMBs = curMBs; break;
          case 2: *addMBs = curMBs; break;
          case 3: *triadMBs = curMBs; break;
       }
    }
    if (doIO) printf(HLINE);

    /* --- Check Results --- */
    checkSTREAMresults(doIO, failure);
    if (doIO) printf(HLINE);

#ifndef VectorSize
    free(c);
    free(b);
    free(a);
#endif

    return 0;
}

# define	M	20

int
checktick()
    {
    int		i, minDelta, Delta;
    double	t1, t2, timesfound[M];

/*  Collect a sequence of M unique time values from the system. */

    for (i = 0; i < M; i++) {
	t1 = mysecond();
	while( ((t2=mysecond()) - t1) < 1.0E-6 )
	    ;
	timesfound[i] = t1 = t2;
	}

/*
 * Determine the minimum difference between these M values.
 * This result will be our estimate (in microseconds) for the
 * clock granularity.
 */

    minDelta = 1000000;
    for (i = 1; i < M; i++) {
	Delta = (int)( 1.0E6 * (timesfound[i]-timesfound[i-1]));
	minDelta = MIN(minDelta, MAX(Delta,0));
	}

   return(minDelta);
}
#undef M


/* A gettimeofday routine to give access to the wall
   clock timer on most UNIX-like systems.  */

#include <sys/time.h>

double wtime()
{
        struct timeval tp;
        struct timezone tzp;
        int i;

        i = gettimeofday(&tp,&tzp);
        return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

void checkSTREAMresults (int doIO, int *failure)
{
	double aj,bj,cj,scalar;
	double asum,bsum,csum;
	double epsilon;
	int	j,k;

    /* reproduce initialization */
	aj = 1.0;
	bj = 2.0;
	cj = 0.0;
    /* a[] is modified during timing check */
	aj = 2.0E0 * aj;
    /* now execute timing loop */
	scalar = 3.0;
	for (k=0; k<NTIMES; k++)
        {
            cj = aj;
            bj = scalar*cj;
            cj = aj+bj;
            aj = bj+scalar*cj;
        }
	aj = aj * (double) (VectorSize);
	bj = bj * (double) (VectorSize);
	cj = cj * (double) (VectorSize);

	asum = 0.0;
	bsum = 0.0;
	csum = 0.0;
	for (j=0; j<VectorSize; j++) {
		asum += a[j];
		bsum += b[j];
		csum += c[j];
	}
#ifdef VERBOSE
	printf ("Results Comparison: \n");
	printf ("        Expected  : %f %f %f \n",aj,bj,cj);
	printf ("        Observed  : %f %f %f \n",asum,bsum,csum);
#endif

#ifndef abs
#define abs(a) ((a) >= 0 ? (a) : -(a))
#endif
	epsilon = 1.e-8;

        *failure = 1;
	if (abs(aj-asum)/asum > epsilon) {
           if (doIO) {
		printf ("Failed Validation on array a[]\n");
		printf ("        Expected  : %f \n",aj);
		printf ("        Observed  : %f \n",asum);
           }
	}
	else if (abs(bj-bsum)/bsum > epsilon) {
           if (doIO) {
		printf ("Failed Validation on array b[]\n");
		printf ("        Expected  : %f \n",bj);
		printf ("        Observed  : %f \n",bsum);
           }
	}
	else if (abs(cj-csum)/csum > epsilon) {
           if (doIO) {
		printf ("Failed Validation on array c[]\n");
		printf ("        Expected  : %f \n",cj);
		printf ("        Observed  : %f \n",csum);
           }
	}
	else {
           *failure = 0;
	   if (doIO) printf ("Solution Validates\n");
	}
}

void tuned_STREAM_Copy()
{
	int j;
#pragma omp parallel for
        for (j=0; j<VectorSize; j++)
            c[j] = a[j];
}

void tuned_STREAM_Scale(double scalar)
{
	int j;
#pragma omp parallel for
	for (j=0; j<VectorSize; j++)
	    b[j] = scalar*c[j];
}

void tuned_STREAM_Add()
{
	int j;
#pragma omp parallel for
	for (j=0; j<VectorSize; j++)
	    c[j] = a[j]+b[j];
}

void tuned_STREAM_Triad(double scalar)
{
	int j;
#pragma omp parallel for
	for (j=0; j<VectorSize; j++)
	    a[j] = b[j]+scalar*c[j];
}

int main(int argc, char *argv[]){
int numprocs, myid, doIO = 0;
double copyLocalMBs, copyMinMBs, copyMaxMBs, copyAvgMBs;
double scaleLocalMBs, scaleMinMBs, scaleMaxMBs, scaleAvgMBs;
double addLocalMBs, addMinMBs, addMaxMBs, addAvgMBs;
double triadLocalMBs, triadMinMBs, triadMaxMBs, triadAvgMBs;
int failure = 0, failureAll = 0, rv, errCount;

#ifdef PARALLEL_MPI
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &myid);
   if (myid == 0) doIO = 1;
#else
   myid = 0;
   numprocs = 1;
   doIO = 1;
#endif

#ifndef VectorSize
   if (argc < 2) {
      VectorSize = N;
   } else {
      VectorSize = atol(argv[1]);
   }
#endif
   rv = stream(&copyLocalMBs, &scaleLocalMBs, &addLocalMBs, &triadLocalMBs,
               &failure);
#ifdef PARALLEL_MPI
   MPI_Reduce( &rv, &errCount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD );
   MPI_Allreduce( &failure, &failureAll, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD );

   MPI_Reduce( &copyLocalMBs, &copyMinMBs, 1, MPI_DOUBLE, MPI_MIN, 0,
               MPI_COMM_WORLD );
   MPI_Reduce( &copyLocalMBs, &copyAvgMBs, 1, MPI_DOUBLE, MPI_SUM, 0,
               MPI_COMM_WORLD );
   MPI_Reduce( &copyLocalMBs, &copyMaxMBs, 1, MPI_DOUBLE, MPI_MAX, 0,
               MPI_COMM_WORLD );
   copyAvgMBs /= numprocs;
   MPI_Reduce( &scaleLocalMBs, &scaleMinMBs, 1, MPI_DOUBLE, MPI_MIN, 0,
               MPI_COMM_WORLD );
   MPI_Reduce( &scaleLocalMBs, &scaleAvgMBs, 1, MPI_DOUBLE, MPI_SUM, 0,
               MPI_COMM_WORLD );
   MPI_Reduce( &scaleLocalMBs, &scaleMaxMBs, 1, MPI_DOUBLE, MPI_MAX, 0,
               MPI_COMM_WORLD );
   scaleAvgMBs /= numprocs;
   MPI_Reduce( &addLocalMBs, &addMinMBs, 1, MPI_DOUBLE, MPI_MIN, 0,
               MPI_COMM_WORLD );
   MPI_Reduce( &addLocalMBs, &addAvgMBs, 1, MPI_DOUBLE, MPI_SUM, 0,
               MPI_COMM_WORLD );
   MPI_Reduce( &addLocalMBs, &addMaxMBs, 1, MPI_DOUBLE, MPI_MAX, 0,
               MPI_COMM_WORLD );
   addAvgMBs /= numprocs;
   MPI_Reduce( &triadLocalMBs, &triadMinMBs, 1, MPI_DOUBLE, MPI_MIN, 0,
               MPI_COMM_WORLD );
   MPI_Reduce( &triadLocalMBs, &triadAvgMBs, 1, MPI_DOUBLE, MPI_SUM, 0,
               MPI_COMM_WORLD );
   MPI_Reduce( &triadLocalMBs, &triadMaxMBs, 1, MPI_DOUBLE, MPI_MAX, 0,
               MPI_COMM_WORLD );
   triadAvgMBs /= numprocs;

   if (doIO && numprocs > 1) {
      printf( "No. of nodes %d; nodes with errors: %d\n", numprocs, errCount );
      printf( "Minimum Copy MB/s %.2f\n", copyMinMBs );
      printf( "Average Copy MB/s %.2f\n", copyAvgMBs );
      printf( "Maximum Copy MB/s %.2f\n", copyMaxMBs );
      printf( "Minimum Scale MB/s %.2f\n", scaleMinMBs );
      printf( "Average Scale MB/s %.2f\n", scaleAvgMBs );
      printf( "Maximum Scale MB/s %.2f\n", scaleMaxMBs );
      printf( "Minimum Add MB/s %.2f\n", addMinMBs );
      printf( "Average Add MB/s %.2f\n", addAvgMBs );
      printf( "Maximum Add MB/s %.2f\n", addMaxMBs );
      printf( "Minimum Triad MB/s %.2f\n", triadMinMBs );
      printf( "Average Triad MB/s %.2f\n", triadAvgMBs );
      printf( "Maximum Triad MB/s %.2f\n", triadMaxMBs );
      if (failureAll)
         printf("Failed Validation on %d node(s)\n", failureAll);
   }
   MPI_Finalize();
#endif

}