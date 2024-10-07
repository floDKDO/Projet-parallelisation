#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#include <mpi.h>
#include <omp.h>


// matrix size and some others
#include "graph.h"

// some utilities to print matrices, etc ...
#include "util.h"

#define DIFFTEMPS(a,b) (((b).tv_sec - (a).tv_sec) + ((b).tv_usec - (a).tv_usec)/1000000.)

int rank, size;



/* 
 * Transitive closure with the Floyd-Warshall algorithm.
 * Works for directed or undirected graphs.
 * https://fr.wikipedia.org/wiki/Algorithme_de_Warshall
 *
 * @param a : the adjacency matrix of the graph
 * @param c : output: the adjacency matrix resulting from tansitive closure   
 */
void warshall(size_t n, int a[n][n], int c[n][n]) 
{

#ifdef PROGRESS
	 size_t percent=0;
#endif
	
	size_t k, i = 0;

	
      MPI_Scatter(a, n*n/size, MPI_INT, a, n*n/size, MPI_INT, 0, MPI_COMM_WORLD);


      #pragma omp parallel 
      {
      	      #pragma omp for private(i) schedule(static)
	      for (k = 0; k < n; k++) {
		    for (i = 0; i < n/size; i++) {
		         if(a[i][k] == 1)
		         {
				  for (size_t j = 0; j < n; j++) {
				  	if(a[k][j] == 1)
						a[i][j] |= a[k][j];
				  }
		         }
		    }
	#ifdef PROGRESS
		if(rank == 0)
		{
			if ((k*100)%n==0) {
			    percent++;
			    fprintf(stderr, "(%3zu%%)", percent); fflush(stderr);fprintf(stderr, "\b\b\b\b\b\b");
			}
		}
	#endif
	      }
     }
     MPI_Gather(a, n*n/size, MPI_INT, c, n*n/size, MPI_INT, 0, MPI_COMM_WORLD);
}

/**
 * main
 */
int main (int argc, char **argv) 
{

	if(MPI_Init(&argc, &argv) != MPI_SUCCESS)
	{
		fprintf(stderr, "erreur\n");
	}

	if(MPI_Comm_rank(MPI_COMM_WORLD, &rank) != MPI_SUCCESS)
	{
		fprintf(stderr, "erreur\n");
	}

	if(MPI_Comm_size(MPI_COMM_WORLD, &size) != MPI_SUCCESS)
	{
		fprintf(stderr, "erreur\n");
	}
	
		char * filename = 0;
	char * input_type = 0;
	parse_options(argc, argv, &filename, &input_type);

      struct timeval tv_init, tv_begin, tv_end;
      gettimeofday (&tv_init, NULL);


      size_t n = 0;
      // read from an adjacency matrix
      // determine size n of matrix
      if (strcmp(input_type, INPUT_TYPE_ADJ)==0) {
          // first get the adjacency matrix size
          n = matrix_lines_from_file(filename);
          fprintf(stderr, "* %s has %ld lines.\n", filename, n);
      }
      // read from a file, pairs of 2 nodes per line
      // determine size n of matrix
      if (strcmp(input_type, INPUT_TYPE_PAIRS)==0) {
          // first get the max integers in the pairs
          n = find_max(filename, "\t");
          // assume values in file may start at 0, so add one to max value. 
          n++;
          fprintf(stderr, "* max value found in %s: %ld (about to build a %ldx%ld adjacency matrix)\n", filename, n, n, n);

      }
      // malloc a VLA as a pointer to an array of n integers
      int (*a)[n] = malloc(sizeof(int[n][n]));
      if (a==0) {
          fprintf(stderr, "* could not alloc %ld bytes. Abort.\n", n*n);
          exit(1);
      }

      // Now load, from one type or another
      if (strcmp(input_type, INPUT_TYPE_ADJ)==0) {
          // read data from a file containing an adjacency matrix
          matrix_from_adj_file(filename, n, a);
      }
      // read from a file, pairs of 2 nodes per line
      if (strcmp(input_type, INPUT_TYPE_PAIRS)==0) {
          // read data from a file containing a list of pairs
          // : adjust the separator "\t", ",", ... depending on your file
          matrix_from_pairs_file(filename, n, a, "\t");
      }

      int (*c)[n] = malloc(sizeof(int[n][n]));
      if (c==0) {
          fprintf(stderr, "* could not alloc %ld bytes. Abort.\n", n*n);
          exit(1);
      }

#ifdef DEBUG
      print_matrix ("a_orig", n , a, 0, n, 0, n, 0, false);
#endif
      
      

     //Compute
     if(rank == 0)
	{
	      fprintf(stderr, "* starting computation (n=%ld) ... ", n);
		fflush(stderr);
	      gettimeofday (&tv_begin, NULL);
	 }
	warshall(n, a, c);
	

	gettimeofday (&tv_end, NULL);
	fprintf(stderr, " done.\n");
	 
	
	if(rank == 0)
	{
      		print_matrix (NULL, n, c, 0, n, 0, n, 0, false);
      	}

#ifdef CCOMP
      size_t ncomps;
      if(rank == 0)
      	comp_t * ccomp = make_ccomp_digraph (n, c, &ncomps);

	free(a);
	free(c);
      fprintf (stderr, "* %ld connected components after make_ccomp_digraph.\n", ncomps);

	if(rank == 0)
	{
		// write the graph transitivly closed in dot format
		char *output_file = malloc(strlen(OUTPUT_TYPE) + strlen(OUTPUT_EXT) + strlen(filename) + 1); 
		sprintf(output_file, "%s%s%s", filename, OUTPUT_TYPE, OUTPUT_EXT);
	      FILE *fgraph_clos = fgraph_clos = fopen (output_file, "w");
	      if (fgraph_clos == NULL) {
		    fprintf (stderr, "Cannot open file %s for writing.\n", output_file);
		    exit (EXIT_FAILURE);
	      }
	      print_dot (fgraph_clos, ccomp, ncomps);
	      fclose (fgraph_clos);
     }
#endif

      //---------------------------------------------------------------------------
      //Execution times
      if(rank == 0)
	{
	      fprintf (stderr, "Init : %lfs, Compute : %lfs\n",
		       DIFFTEMPS (tv_init, tv_begin), DIFFTEMPS (tv_begin, tv_end));
	}
	
	MPI_Finalize();

      return 0;
}

