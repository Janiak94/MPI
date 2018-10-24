#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#define MASTER 0
#define VISITED 1
#define UNVISITED 0
#define MAX_NUM_NODES 1000
#define MAX_NUM_ARCS 100000
#define MAX_FILE_SIZE 1567070885
#define INF 18446744073709551615

/* handles errors */
void error_handler(char msg[], int error, int id){
	static int bcast_id;
	if(msg != NULL){
		bcast_id = id;
	}
	MPI_Bcast(&error, 1, MPI_INT, bcast_id, MPI_COMM_WORLD);
	if(error != 0){
		if(id == 0){
			printf("process %d exited with message: %s, code: %d\n\n", bcast_id, msg, error);
		}
		MPI_Finalize();
		exit(error);
	}
}

/* describes an arc */
typedef struct {
	int node_s, node_e, weight;
}Arc;

/* parse the input arguments */
int parse_args(
	int argc,
	char *argv[],
	size_t *start_node,
	size_t *end_node,
	char *file_name[50]
){	
	if(argc != 4){
		return 1;
	}
	*start_node = atoi(argv[1]);
	*end_node = atoi(argv[2]);
	if(*start_node == *end_node){
		return 2;
	}
	strcpy(file_name, argv[3]);
	
	return 0;
}

/* read and parse the input file */
void parse_file(char **am, int *num_arcs, FILE *fp){
	char *buffer = (char*) malloc(MAX_FILE_SIZE), *end_ptr = buffer;
	size_t bytes_read, current_line = 0;
	bytes_read = fread(buffer, 1, MAX_FILE_SIZE, fp);
	while(end_ptr != NULL && *end_ptr != '\0'){
		/* read vertex 1 */
		int node1 = (int) strtol(end_ptr, &end_ptr, 10);

		/* read vertex 2 */
		int node2 = (int) strtol(end_ptr, &end_ptr, 10);

		if(node1 == 0 && node2 == 0){
			break;
		}

		/* read weight */
		int weight = (int) strtol(end_ptr, &end_ptr, 10);
		
		am[node1][node2] = weight;
		++current_line;
	}
	*num_arcs = current_line;
	free(buffer);
}

/* master process */
void main_master(int num_mpi_proc, int argc, char *argv[]){
	//vector of visitd nodes. 1 indicate visited, 0 unvisited
	int *visited = (int*) calloc(MAX_NUM_NODES, sizeof(int));
	size_t start_node, end_node;
	char file_name[50], **am, *am_entries;
	FILE *fp;
		
	int tag, error = parse_args(argc, argv, &start_node, &end_node, &file_name);
	/* broadcast error code and handle it */
	error_handler("error while parsing args", error, MASTER);

	/* initialize adjacency matrix */
	am = (char**) malloc(sizeof(char*) * MAX_NUM_NODES);
	am_entries = (char*) calloc(MAX_NUM_NODES*MAX_NUM_NODES,1); 
	for(int i = 0, j = 0; i < MAX_NUM_NODES; ++i, j+=MAX_NUM_NODES){
		am[i] = am_entries + j;
	}

	fp = fopen(file_name, "r");
	/* check for error while opening file */
	error = (fp == NULL ? 1 : 0);
	error_handler("error opening file", error, MASTER);
	int test;
	parse_file(am, &test, fp);
	fclose(fp);

	printf("%d\n", test);
	
	free(visited);
	free(am);
	free(am_entries);
}

void main_worker(int num_mpi_proc, int mpi_rank){
	int error, tag;
	MPI_Status status;

	/* check wether the arguments got accepted or not */
	error_handler(NULL, error, mpi_rank);

	/* check if the file could be opened or not */
	error_handler(NULL, error, mpi_rank);
}

int main(int argc, char *argv[]){
	MPI_Init(&argc, &argv);
	
	int num_mpi_proc, mpi_rank;
	MPI_Comm_size(MPI_COMM_WORLD, &num_mpi_proc);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	
	if(mpi_rank == 0){
		main_master(num_mpi_proc, argc, argv);
	}else{
		for(int i = 1; i < num_mpi_proc; ++i){
			if(mpi_rank == i){
				main_worker(num_mpi_proc, mpi_rank);
			}
		}
	}

	MPI_Finalize();
	return 0;
}
