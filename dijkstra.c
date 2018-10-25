#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#define MASTER 0
#define VISITED 1
#define UNVISITED 0
#define MAX_NUM_NODES 10000
#define MAX_NUM_ARCS 100000
#define MAX_FILE_SIZE 1567070885
#define INF 18446744073709551615

int mpi_rank, num_mpi_proc;
int num_nodes, **am, *am_entries;

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
	int *start_node,
	int *end_node,
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
void parse_file(int **am, FILE *fp){
	char *buffer = (char*) malloc(MAX_FILE_SIZE), *end_ptr = buffer;
	size_t bytes_read;
	bytes_read = fread(buffer, 1, MAX_FILE_SIZE-1, fp);
	num_nodes = 0;
	while(end_ptr != NULL && *end_ptr != '\0'){
		/* read nodes and weight */
		int node1 = (int) strtol(end_ptr, &end_ptr, 10);
		int node2 = (int) strtol(end_ptr, &end_ptr, 10);
		int weight = (int) strtol(end_ptr, &end_ptr, 10);
		if(node1 == 0 && node2 == 0){
			break;
		}
		am[node1][node2] = weight;
		if(node1+1 > num_nodes){
			num_nodes = node1+1;
		}else if(node2+1 > num_nodes){
			num_nodes = node2+1;
		}
	}
	free(buffer);
}

/* dijkstra */
size_t dijkstra(const int start_node, int *end_node){
	char is_visited[MAX_NUM_NODES];
	size_t distance[MAX_NUM_NODES];
	int previous_node[MAX_NUM_NODES];
	size_t current_node_value;
	int cheapest_path;

	for(int i = mpi_rank; i < num_nodes; i+=num_mpi_proc){
		is_visited[i] = 0;
		distance[i] = am[start_node][i];
		previous_node[i] = start_node;
	}
	is_visited[start_node]=1;
	previous_node[start_node]=-1;

	/* go through all nodes */
	for(int j = 0; j < num_nodes; ++j){
		current_node_value = INF;
		
		for(int i = mpi_rank; i < num_nodes; i+=num_mpi_proc){
			if(is_visited[i] == 0 && distance[i] < current_node_value){
				current_node_value = distance[i];
				cheapest_path = i;
			}
		}

		struct{
			int current_node_value, cheapest_path;
		}p, p_global;
		p.current_node_value = current_node_value;
		p.cheapest_path = cheapest_path;

		MPI_Allreduce(&p, &p_global, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);

		cheapest_path = p_global.cheapest_path;
		distance[cheapest_path] = p_global.current_node_value;
		is_visited[cheapest_path]=1;

		for(int i = mpi_rank; i < num_nodes; i+=num_mpi_proc){
			if(is_visited[i] == 0 && distance[i] > distance[cheapest_path] + am[cheapest_path][i]){
				distance[i] = distance[cheapest_path] + am[cheapest_path][i];
				previous_node[i] = cheapest_path;
			}
		}
	}

	if(mpi_rank == MASTER){
		return distance[*end_node];
	}
	return 1;
}

struct{
	int start_node, num_nodes;
}args;

/* master process */
void main_master(int num_mpi_proc, int argc, char *argv[]){
	int start_node, end_node;
	char file_name[50];
	FILE *fp;
	
	int tag, error = parse_args(argc, argv, &start_node, &end_node, &file_name);
	/* broadcast error code and handle it */
	error_handler("error while parsing args", error, MASTER);

	/* initialize adjacency matrix */
	am = (int**) malloc(sizeof(int*) * MAX_NUM_NODES);
	am_entries = (int*) calloc(MAX_NUM_NODES*MAX_NUM_NODES,sizeof(int)); 
	for(int i = 0, j = 0; i < MAX_NUM_NODES; ++i, j+=MAX_NUM_NODES){
		am[i] = am_entries + j;
	}
	for(int i = 0; i < MAX_NUM_NODES*MAX_NUM_NODES; ++i){
		am_entries[i] = 1001;
	}

	fp = fopen(file_name, "r");
	/* check for error while opening file */
	error = (fp == NULL ? 1 : 0);
	error_handler("error opening file", error, MASTER);

	parse_file(am, fp);
	fclose(fp);

	/* broadcast the adjacency matrix */
	MPI_Bcast(am_entries, MAX_NUM_NODES*MAX_NUM_NODES, MPI_INT, MASTER, MPI_COMM_WORLD);

	args.start_node = start_node;
	args.num_nodes = num_nodes;

	/* broadcast the number of nodes in file, start node and end node */
	MPI_Bcast(&args, 1, MPI_2INT, MASTER, MPI_COMM_WORLD);

	size_t price = dijkstra(start_node, &end_node);
	
	printf("%lu\n", price);

	free(am);
	free(am_entries);
}

void main_worker(int num_mpi_proc, int mpi_rank){
	int error, tag;
	MPI_Status status;

	/* check wether the arguments got accepted or not */
	error_handler(NULL, error, mpi_rank);

	/* initialize adjacency matrix */
	am = (int**) malloc(sizeof(int*) * MAX_NUM_NODES);
	am_entries = (int*) calloc(MAX_NUM_NODES*MAX_NUM_NODES,sizeof(int)); 
	for(int i = 0, j = 0; i < MAX_NUM_NODES; ++i, j+=MAX_NUM_NODES){
		am[i] = am_entries + j;
	}

	/* check if the file could be opened or not */
	error_handler(NULL, error, mpi_rank);

	/* broadcast the adjacency matrix */
	MPI_Bcast(am_entries, MAX_NUM_NODES*MAX_NUM_NODES, MPI_INT, MASTER, MPI_COMM_WORLD);

	/* get the number of nodes in file and start node */
	MPI_Bcast(&args, 1, MPI_2INT, MASTER, MPI_COMM_WORLD);
	num_nodes = args.num_nodes;

	/* start dijkstra */
	dijkstra(args.start_node, NULL);

	free(am);
	free(am_entries);
}

int main(int argc, char *argv[]){
	MPI_Init(&argc, &argv);
	
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
