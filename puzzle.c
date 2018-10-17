#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/time.h>

typedef struct node{
	int state[16];
	int g;
	int f;
	// move applied to reach node
	int operator;
} node;

/**
 * Global Variables
 */

#define GRID_SQUARES 16
#define MAX_MOVES 4

/**
 * Helper array of the x and y positions of each tile
 * Largely redundant since tile_dist was implemented but no reason not to use it
 */
int tile_positions[GRID_SQUARES][2] = {
	{0, 0}, {1, 0}, {2, 0}, {3, 0},
	{0, 1}, {1, 1}, {2, 1}, {3, 1},
	{0, 2}, {1, 2}, {2, 2}, {3, 2},
	{0, 3}, {1, 3}, {2, 3}, {3, 3}
};

// stores the manhattan distance between every possible pair of tiles
int tile_dist[GRID_SQUARES][GRID_SQUARES];

// used to track the position of the blank in a state,
// so it doesn't have to be searched every time we check if an operator is applicable
// When we apply an operator, blank_pos is updated
int blank_pos;

// Initial node of the problem
node initial_node;

// Statistics about the number of generated and expendad nodes
unsigned long generated;
unsigned long expanded;

/**
 * The id of the four available actions for moving the blank (empty slot). e.x.
 * Left: moves the blank to the left, etc.
 */

#define LEFT 0
#define RIGHT 1
#define UP 2
#define DOWN 3

// index of opposite directions
int opposite_dir[MAX_MOVES] = { 1, 0, 3, 2 };

/*
 * Helper arrays for the applicable function
 * applicability of operators: 0 = left, 1 = right, 2 = up, 3 = down
 */
int ap_opLeft[]	= { 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1 };
int ap_opRight[] = { 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0 };
int ap_opUp[]	= { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
int ap_opDown[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 };
int *ap_ops[] = { ap_opLeft, ap_opRight, ap_opUp, ap_opDown };

/* print state */
void print_state( int* s )
{
	int i;

	for( i = 0; i < 16; i++ )
		printf( "%2d%c", s[i], ((i+1) % 4 == 0 ? '\n' : ' ') );
}

void printf_comma (long unsigned int n) {
		if (n < 0) {
				printf ("-");
				printf_comma (-n);
				return;
		}
		if (n < 1000) {
				printf ("%lu", n);
				return;
		}
		printf_comma (n/1000);
		printf (",%03lu", n%1000);
}

/* returns the manhattan distance of a single tile to its final destination */
int manhattan_tile( int tile_pos, int tile_dest )
{
	// uses array 'tile_positions' so they don't have to be calculated every time
	int x_pos = tile_positions[tile_pos][0];
	int y_pos = tile_positions[tile_pos][1];
	int x_dest = tile_positions[tile_dest][0];
	int y_dest = tile_positions[tile_dest][1];

	return( abs(x_pos - x_dest) + abs(y_pos - y_dest) );
}

/* calculates the mantattan distance between every tile pair */
void populate_tile_dist()
{
	int i;
	int j;
	for( i = 0; i < GRID_SQUARES; i++  )
	{
		for( j = 0; j < GRID_SQUARES; j++  )
		{
			tile_dist[i][j] = manhattan_tile(i,j);
		}
	}
}

/* return the sum of manhattan distances from state to goal */
int manhattan( int* state )
{
	int sum = 0;

	int i;
	for( i = 0; i < GRID_SQUARES; i++  )
	{
		// don't count if blank tile or tile is already in position
		if (state[i] != 0 && state[i] != i )
		{
			sum += tile_dist[i][state[i]];
		}
	}

	return( sum );
}

/* return 1 if op is applicable in state, otherwise return 0 */
int applicable( int op )
{
			 	return( ap_ops[op][blank_pos] );
}


/* apply operator */
void apply( node* n, int op )
{
	int t;

	//find tile that has to be moved given the op and blank_pos
	t = blank_pos + (op == 0 ? -1 : (op == 1 ? 1 : (op == 2 ? -4 : 4)));

	//apply op
	n->state[blank_pos] = n->state[t];
	n->state[t] = 0;

	//update blank pos
	blank_pos = t;
}

/* returns the lesser of the two values given */
int minimum(int x, int y) {
	return (x < y ? x : y);
}

/* Recursive IDA */
node* ida( node* node, int threshold, int* newThreshold )
{
	struct node* r;

	int prev_f;
	int prev_operator;
	int hueristic;

	int i;
	for( i = 0; i < MAX_MOVES; i++ )
	{
		// only consider valid moves
		if (!applicable(i))
		{
			continue;
		}

		// prevents ida from reversing last move
		if ( node->operator > 0)
		{
			if ( i == opposite_dir[node->operator] )
			{
				continue;
			}
		}

		// move valid and new node generated
		generated++;

		// store current values for reversal later
		prev_f = node->f;
		prev_operator = node->operator;

		// applies move to node
		apply(node, i);

		// stores move used to reach this node
		node->operator = i;

		// stores manhattan value so we only need to calculate it once
		hueristic = manhattan(node->state);

		// update cost and heuristic
		node->g = node->g + 1;
		node->f = node->g + hueristic;

		if( node->f > threshold )
		{
			*newThreshold = minimum(node->f, *newThreshold);
		}
		else
		{
			if (hueristic == 0){
				// goal state found, return node
				r = node;
				return r;
			}

			// if goal not found recursively repeat
			expanded++;
			r = ida(node, threshold, newThreshold);

			// return r if goal state found
			if (r != NULL)
			{
				return r;
			}
		}

		// reverse move on node before next iteration
		apply(node, opposite_dir[i]);

		node->g = node->g - 1;

		node->f = prev_f;
		node->operator = prev_operator;
	}

	return( NULL );
}


/* main IDA control loop */
int IDA_control_loop(	){
	node* r = NULL;

	int threshold;

	/* initialize statistics */
	generated = 0;
	expanded = 0;

	/* compute initial threshold B */
	initial_node.f = threshold = manhattan( initial_node.state );

	printf( "Initial Estimate = %d\nThreshold = ", threshold );

	int newThreshold;

	while(r == NULL)
	{
		// set newThreshold to infinity
		newThreshold = INT_MAX;

		r = ida(&initial_node, threshold, &newThreshold);

		if( r == NULL) {
			// threshold increased, print new threshold
			threshold = newThreshold;
			printf("%d ", threshold);
		}

	}

	if(r)
		return r->g;
	else
		return -1;
}


static inline float compute_current_time()
{
	struct rusage r_usage;

	getrusage( RUSAGE_SELF, &r_usage );
	float diff_time = (float) r_usage.ru_utime.tv_sec;
	diff_time += (float) r_usage.ru_stime.tv_sec;
	diff_time += (float) r_usage.ru_utime.tv_usec / (float)1000000;
	diff_time += (float) r_usage.ru_stime.tv_usec / (float)1000000;
	return diff_time;
}

int main( int argc, char **argv )
{
	int i, solution_length;

	/* check we have a initial state as parameter */
	if( argc != 2 )
	{
		fprintf( stderr, "usage: %s \"<initial-state-file>\"\n", argv[0] );
		return( -1 );
	}


	/* read initial state */
	FILE* initFile = fopen( argv[1], "r" );
	char buffer[256];

	if( fgets(buffer, sizeof(buffer), initFile) != NULL ){
		char* tile = strtok( buffer, " " );
		for( i = 0; tile != NULL; ++i )
			{
				initial_node.state[i] = atoi( tile );
				blank_pos = (initial_node.state[i] == 0 ? i : blank_pos);
				tile = strtok( NULL, " " );
			}
	}
	else{
		fprintf( stderr, "Filename empty\"\n" );
		return( -2 );

	}

	if( i != 16 )
	{
		fprintf( stderr, "invalid initial state\n" );
		return( -1 );
	}

	/* initialize the initial node */
	initial_node.g=0;
	initial_node.f=0;

	// initial node has no previous action
	initial_node.operator = -1;

	print_state( initial_node.state );

	// populates tile_dist array
	populate_tile_dist();

	/* solve */
	float t0 = compute_current_time();

	solution_length = IDA_control_loop();

	float tf = compute_current_time();

	/* report results */
	printf( "\nSolution = %d\n", solution_length);
	printf( "Generated = ");
	printf_comma(generated);
	printf("\nExpanded = ");
	printf_comma(expanded);
	printf( "\nTime (seconds) = %.2f\nExpanded/Second = ", tf-t0 );
	printf_comma((unsigned long int) expanded/(tf+0.00000001-t0));
	printf("\n\n");

	/* aggregate all executions in a file named report.dat, for marking purposes */
	FILE* report = fopen( "report.dat", "a" );

	fprintf( report, "%s", argv[1] );
	fprintf( report, "\n\tSoulution = %d, Generated = %lu, Expanded = %lu", solution_length, generated, expanded);
	fprintf( report, ", Time = %f, Expanded/Second = %f\n\n", tf-t0, (float)expanded/(tf-t0));
	fclose(report);

	return( 0 );
}
