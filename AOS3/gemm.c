/*code from https://github.com/flame/how-to-optimize-gemm for gemm benchmark*/
/* Create macros so that the matrices are stored in column-major order */

/*
In the test driver, there is a loop "for ( p=PFIRST; p<= PLAST; p+= PINC )"
The below parameters set this range of values that p takes on
*/
#define PFIRST 1000
#define PLAST  800
#define PINC   40

/*
In the test driver, the m, n, and k dimensions are set to the below
values.  If the value equals "-1" then that dimension is bound to the
index p, given above.
*/

#define M -1
#define N -1
#define K -1

/*
In the test driver, each experiment is repeated NREPEATS times and
the best time from these repeats is used to compute the performance
*/

#define NREPEATS 1

/*
Matrices A, B, and C are stored in two dimensional arrays with
row dimensions that are greater than or equal to the row dimension
of the matrix.  This row dimension of the array is known as the
"leading dimension" and determines the stride (the number of
double precision numbers) when one goes from one element in a row
to the next.  Having this number larger than the row dimension of
the matrix tends to adversely affect performance.  LDX equals the
leading dimension of the array that stores matrix X.  If LDX=-1
then the leading dimension is set to the row dimension of matrix X.
*/

#define LDA 8000
#define LDB 8000
#define LDC 8000


#define A(i,j) a[ (j)*lda + (i) ]
#define B(i,j) b[ (j)*ldb + (i) ]
#define C(i,j) c[ (j)*ldc + (i) ]

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

static double gtod_ref_time_sec = 0.0;

/* Adapted from the bl2_clock() routine in the BLIS library */

double dclock()
{
        double         the_time, norm_sec;
        struct timeval tv;

        gettimeofday( &tv, NULL );

        if ( gtod_ref_time_sec == 0.0 )
                gtod_ref_time_sec = ( double ) tv.tv_sec;

        norm_sec = ( double ) tv.tv_sec - gtod_ref_time_sec;

        the_time = norm_sec + tv.tv_usec * 1.0e-6;

        return the_time;
}

double compare_matrices( int m, int n, double *a, int lda, double *b, int ldb )
{
  int i, j;
  double max_diff = 0.0, diff;

#define abs( x ) ( (x) < 0.0 ? -(x) : (x) )
  for ( j=0; j<n; j++ )
    for ( i=0; i<m; i++ ){
      diff = abs( A( i,j ) - B( i,j ) );
      max_diff = ( diff > max_diff ? diff : max_diff );
    }

  return max_diff;
}

void random_matrix( int m, int n, double *a, int lda )
{
  double drand48();
  int i,j;

  for ( j=0; j<n; j++ )
    for ( i=0; i<m; i++ )
      A( i,j ) = 2.0 * drand48( ) - 1.0;
}

void print_matrix( int m, int n, double *a, int lda )
{
  int i, j;

  for ( j=0; j<n; j++ ){
    for ( i=0; i<m; i++ )
      printf("%le ", A( i,j ) );
    printf("\n");
  }
  printf("\n");
}


void REF_MMult( int m, int n, int k, double *a, int lda,
                                    double *b, int ldb,
                                    double *c, int ldc )
{
  int i, j, p;

  for ( i=0; i<m; i++ ){
    for ( j=0; j<n; j++ ){
      for ( p=0; p<k; p++ ){
	C( i,j ) = C( i,j ) +  A( i,p ) * B( p,j );
      }
    }
  }
}



void copy_matrix( int m, int n, double *a, int lda, double *b, int ldb )
{
  int i, j;

  for ( j=0; j<n; j++ )
    for ( i=0; i<m; i++ )
      B( i,j ) = A( i,j );
}


/* Routine for computing C = A * B + C */

void AddDot1x4( int, double *, int,  double *, int, double *, int );

void MY_MMult( int m, int n, int k, double *a, int lda,
                                    double *b, int ldb,
                                    double *c, int ldc )
{
  int i, j;

  for ( j=0; j<n; j+=4 ){        /* Loop over the columns of C, unrolled by 4 */
    for ( i=0; i<m; i+=1 ){        /* Loop over the rows of C */
      /* Update C( i,j ), C( i,j+1 ), C( i,j+2 ), and C( i,j+3 ) in
	 one routine (four inner products) */

      AddDot1x4( k, &A( i,0 ), lda, &B( 0,j ), ldb, &C( i,j ), ldc );
    }
  }
}


void AddDot1x4( int k, double *a, int lda,  double *b, int ldb, double *c, int ldc )
{
  /* So, this routine computes four elements of C:
           C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 ).
     Notice that this routine is called with c = C( i, j ) in the
     previous routine, so these are actually the elements
           C( i, j ), C( i, j+1 ), C( i, j+2 ), C( i, j+3 )

     in the original matrix C.
     We next use indirect addressing */

  int p;
  register double
    /* hold contributions to
       C( 0, 0 ), C( 0, 1 ), C( 0, 2 ), C( 0, 3 ) */
       c_00_reg,   c_01_reg,   c_02_reg,   c_03_reg,
    /* holds A( 0, p ) */
       a_0p_reg;
  double
    /* Point to the current elements in the four columns of B */
    *bp0_pntr, *bp1_pntr, *bp2_pntr, *bp3_pntr;

  bp0_pntr = &B( 0, 0 );
  bp1_pntr = &B( 0, 1 );
  bp2_pntr = &B( 0, 2 );
  bp3_pntr = &B( 0, 3 );

  c_00_reg = 0.0;
  c_01_reg = 0.0;
  c_02_reg = 0.0;
  c_03_reg = 0.0;

  for ( p=0; p<k; p+=4 ){
    a_0p_reg = A( 0, p );

    c_00_reg += a_0p_reg * *bp0_pntr;
    c_01_reg += a_0p_reg * *bp1_pntr;
    c_02_reg += a_0p_reg * *bp2_pntr;
    c_03_reg += a_0p_reg * *bp3_pntr;

    a_0p_reg = A( 0, p+1 );

    c_00_reg += a_0p_reg * *(bp0_pntr+1);
    c_01_reg += a_0p_reg * *(bp1_pntr+1);
    c_02_reg += a_0p_reg * *(bp2_pntr+1);
    c_03_reg += a_0p_reg * *(bp3_pntr+1);

    a_0p_reg = A( 0, p+2 );

    c_00_reg += a_0p_reg * *(bp0_pntr+2);
    c_01_reg += a_0p_reg * *(bp1_pntr+2);
    c_02_reg += a_0p_reg * *(bp2_pntr+2);
    c_03_reg += a_0p_reg * *(bp3_pntr+2);

    a_0p_reg = A( 0, p+3 );

    c_00_reg += a_0p_reg * *(bp0_pntr+3);
    c_01_reg += a_0p_reg * *(bp1_pntr+3);
    c_02_reg += a_0p_reg * *(bp2_pntr+3);
    c_03_reg += a_0p_reg * *(bp3_pntr+3);

    bp0_pntr+=4;
    bp1_pntr+=4;
    bp2_pntr+=4;
    bp3_pntr+=4;
  }

  C( 0, 0 ) += c_00_reg;
  C( 0, 1 ) += c_01_reg;
  C( 0, 2 ) += c_02_reg;
  C( 0, 3 ) += c_03_reg;
}



int main()
{
  int
    p,
    m, n, k,
    lda, ldb, ldc,
    rep;

  double
    dtime, dtime_best,
    gflops,
    diff;

  double
    *a, *b, *c, *cref, *cold;

  dtime = dclock();
  printf( "MY_MMult = [\n" );

  for ( p=PFIRST; p<=PFIRST; p+=PINC ){
    m = ( M == -1 ? p : M );
    n = ( N == -1 ? p : N );
    k = ( K == -1 ? p : K );

    gflops = 2.0 * m * n * k * 1.0e-09;

    lda = ( LDA == -1 ? m : LDA );
    ldb = ( LDB == -1 ? k : LDB );
    ldc = ( LDC == -1 ? m : LDC );

    /* Allocate space for the matrices */
    /* Note: I create an extra column in A to make sure that
       prefetching beyond the matrix does not cause a segfault */
    a = ( double * ) malloc( lda * (k+1) * sizeof( double ) );
    b = ( double * ) malloc( ldb * n * sizeof( double ) );
    c = ( double * ) malloc( ldc * n * sizeof( double ) );
    cold = ( double * ) malloc( ldc * n * sizeof( double ) );
    cref = ( double * ) malloc( ldc * n * sizeof( double ) );

    /* Generate random matrices A, B, Cold */
    random_matrix( m, k, a, lda );
    random_matrix( k, n, b, ldb );
    random_matrix( m, n, cold, ldc );

    copy_matrix( m, n, cold, ldc, cref, ldc );

    /* Run the reference implementation so the answers can be compared */

    REF_MMult( m, n, k, a, lda, b, ldb, cref, ldc );
    

    free( a );
    free( b );
    free( c );
    free( cold );
    free( cref );
  }
  dtime = dclock() - dtime;

  printf( "%f to execute \n", dtime );
  fflush(stdout);


  exit( 0 );
}
