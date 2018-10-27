#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#define MASTER 0
#define TRUE 1
#define FALSE 0
#define MAX_NUMB_OF_VERTS 100000
#define MAX_FILE_SIZE 1567070885
#define INF 2147483647
#define MAX_DEGREE 1000

/* describes a single vertex 
 * has information about which vertices that are connected
 * and their respective weight */
typedef struct{
	int id;
	int verts[MAX_DEGREE];
	int weights[MAX_DEGREE];
	int price;
	int is_visited;
	int length;
	int previous_vert;
}vertex_t;

/* global variables */
int start_vert = 4, dest_vert = 5;
int mpi_rank, num_mpi_proc, tag;
MPI_Status status;
size_t number_of_verts, number_of_arcs, degree;
vertex_t *verti, *verti_local;
int *path;
char *file_name;
int first_vert, last_vert;
int current_vert, current_price;
int local_min_price;
int local_min_id;
int local_min_previous;
char local_min_is_dirty;
int dest_rank;

/* load data into file and assign nodes */
void load_data(FILE *fp){
	/* let master parse file */
	if(mpi_rank == MASTER){
		verti = (vertex_t*) malloc(sizeof(vertex_t)*MAX_NUMB_OF_VERTS);
		char *buffer = (char*) malloc(MAX_FILE_SIZE), *next_entry = buffer;
		size_t bytes_read, current_line = 0;
		bytes_read = fread(buffer, 1, MAX_FILE_SIZE, fp);
		buffer[bytes_read] = '\0';
		long vert0, vert1, weight;
		int * counter = calloc(MAX_NUMB_OF_VERTS, sizeof(int));
		int current_vert = -1;
		while(*next_entry != '\0'){
			vert0  = strtol(next_entry, &next_entry, 10);
			vert1  = strtol(next_entry, &next_entry, 10);
			weight = strtol(next_entry, &next_entry, 10);
			if(vert0 == 0 && vert1 == 0){
				break;
			}
			verti[vert0].verts[counter[vert0]] = vert1;
			verti[vert0].weights[counter[vert0]++] = weight;

			if (current_vert != vert0) {
				verti[vert0].id = vert0;
				number_of_verts++;
				current_vert = vert0;
			}
		}
		degree = counter[0];
		number_of_arcs = degree*number_of_verts;
		free(buffer);
		free(counter);
	}
	/* bcast number of verts from master to slaves */
	MPI_Bcast(&number_of_verts, 1, MPI_INT, MASTER, MPI_COMM_WORLD);

	first_vert = mpi_rank*number_of_verts/num_mpi_proc;
	last_vert = (mpi_rank+1)*number_of_verts/num_mpi_proc-1;
	int chunk_size = last_vert-first_vert+1;
	dest_rank = dest_vert/chunk_size;

	/* assign vertices to slaves and master */
	verti_local = malloc(sizeof(vertex_t)*number_of_verts);
	if(mpi_rank == MASTER){
		for(int i = num_mpi_proc-1; i >= 0; --i){
			int first_vert_block = i*number_of_verts/num_mpi_proc;
			int last_vert_block = (i+1)*number_of_verts/num_mpi_proc-1;
			for(int j = 0; j < number_of_verts; ++j){
				int count = 0;
				for(int k = 0; k < degree; ++k){
					if(verti[j].verts[k] >= first_vert_block && verti[j].verts[k] <= last_vert_block){
						verti_local[j].verts[count] = verti[j].verts[k];
						verti_local[j].weights[count++] = verti[j].weights[k];
					}
				}
				verti_local[j].length = count;
			}
			/* if not master, send to slave */
			if(i){
				MPI_Send(&verti_local[0].verts[0],
				number_of_verts*(2*MAX_DEGREE+5), MPI_INT, i, tag, MPI_COMM_WORLD);
			}
		}
		free(verti);
	}else{
		/* recieve from master */
		MPI_Recv(&verti_local[0].verts[0], number_of_verts*(2*MAX_DEGREE+5),
		MPI_INT, MASTER, tag, MPI_COMM_WORLD, &status);
	}
	/* initiate vertices for dijkstra */
	for (int i = 0; i < number_of_verts; i++) {
		verti_local[i].price = INF;
		verti_local[i].is_visited = FALSE;
	}
}

/* calculate the updated node prices according to the dijkstra scheme */
static inline void update_price() {
	for (int i = 0; i < verti_local[current_vert].length; ++i) {
		int dest = verti_local[current_vert].verts[i];
		int weight = verti_local[current_vert].weights[i];
		if (verti_local[dest].is_visited == FALSE) {
			if (verti_local[dest].price > current_price + weight) {
				verti_local[dest].price = current_price + weight;
				verti_local[dest].previous_vert = current_vert;
				if (verti_local[dest].price < local_min_price) {
					local_min_price = verti_local[dest].price;
					local_min_id = dest;
					local_min_previous = current_vert;
				}
			}
		}
	}
}

/* find the cheapest vertex among the ones the MPI process are assigned */
static inline void update_min() {
	if (local_min_is_dirty) {
		local_min_price = INF;
		for (int i = first_vert; i <= last_vert; i++) {
			if (verti_local[i].is_visited == FALSE && verti_local[i].price < local_min_price) {
				local_min_price = verti_local[i].price;
				local_min_id = i;
				local_min_previous = verti_local[i].previous_vert;
			}
		}
		local_min_is_dirty = FALSE;
	}
}

/* if the new, globally cheapest, vertex is on of the assigned
 * vertices to the process, indicate the local min as dirty.
 * set local min to INF so we wont pick that one as our next vertex */
static inline void set_visited() {
	if (current_vert >= first_vert && current_vert <= last_vert) {
		verti_local[current_vert].is_visited = TRUE;
		local_min_price = INF;
		local_min_is_dirty = TRUE;
	}
}

void dijkstra(){
	/* initiate process-local variables that keeps track of their local minimum and so on */
	local_min_price = INF;
	local_min_is_dirty = FALSE;
	current_vert = start_vert;
	current_price = 0;
	for(int i = 0; i < number_of_verts; ++i){
		/* calculate assigned vertex prices and which one is the minimum */
		update_price();
		update_min();
		
		struct{	
			int price;
			int id;
		} pair, pair_global;
		pair.price = local_min_price;
		pair.id = mpi_rank;
		/* find global minimum and which process that is responsible for it */
		MPI_Allreduce(&pair, &pair_global, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);
		// save reduced global to current_vert and current_price
		current_price = pair_global.price;
		current_vert = local_min_id;
		int prev = local_min_previous;
		
		/* let the process responsible for the global minimum vertex broadcast
 		 * the relevant information to all other processes */
		MPI_Bcast(&current_vert, 1, MPI_INT, pair_global.id, MPI_COMM_WORLD);
		MPI_Bcast(&prev, 1, MPI_INT, pair_global.id, MPI_COMM_WORLD);
		
		/* set which vertex we came from to the new vertex so we can trace the path */
		verti_local[current_vert].previous_vert = prev;
		set_visited();

		/* termination criterea, is all other vertex prices greater than the destination, then quit */
		int terminate = FALSE;
		if (mpi_rank == dest_rank) {
			if (current_price >= verti_local[dest_vert].price) {
				terminate = TRUE;
			}
		}
		MPI_Bcast(&terminate, 1, MPI_INT, dest_rank, MPI_COMM_WORLD);
		if (terminate) {
			break;
		}
	}
	/* let the process responsible for the destination vertex print the result */
	if(dest_vert >= first_vert && dest_vert <= last_vert){
		path = malloc(MAX_DEGREE*sizeof(int));
		int count = 0;
		int on_vert = dest_vert;
		while(on_vert != start_vert){
			path[count++] = on_vert;
			on_vert = verti_local[on_vert].previous_vert;
		}
		printf("Shortest path of length %d\n", verti_local[dest_vert].price);
		printf("%d ", start_vert);
		for(int i = count-1; i >= 0; --i){
			printf("-> %d ", path[i]);
		}
		printf("\n");
	}
}

/* parse input arguments */
void parse_args(int argc, char *argv[]){
	if(argc != 4){
		printf("wrong number of args\n");
		exit(1);
	}
	start_vert = atoi(argv[1]);
	dest_vert = atoi(argv[2]);
	file_name = argv[3];
}

int main(int argc, char *argv[]){
	/* initiate MPI */
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &num_mpi_proc);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

	parse_args(argc, argv);
	/* only master opens the file */
	FILE *fp = NULL;
	if(mpi_rank == MASTER){
		fp = fopen(file_name,"r");
		if(fp == NULL){
			printf("could not open file with name %s\n",file_name);
			exit(1);
		}
	}
	load_data(fp);
	if(mpi_rank == MASTER)
		fclose(fp);
	dijkstra();
	MPI_Finalize();
	exit(0);
}
